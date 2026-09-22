package main

import (
	"context"
	"crypto/rand"
	"crypto/sha256"
	"database/sql"
	"encoding/hex"
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"net/url"
	"strings"
	"time"
	"unicode"
	"unicode/utf8"

	_ "github.com/jackc/pgx/v5/stdlib"
	"github.com/msetaro/aftershock/tools/match/contracts"
)

func openBackendDB(ctx context.Context, dsn string) (*sql.DB, error) {
	if dsn == "" {
		return nil, errors.New("database configuration required")
	}
	db, err := sql.Open("pgx", dsn)
	if err != nil {
		return nil, err
	}
	db.SetMaxOpenConns(8)
	db.SetMaxIdleConns(4)
	db.SetConnMaxLifetime(30 * time.Minute)
	tx, err := db.BeginTx(ctx, nil)
	if err == nil {
		defer tx.Rollback()
		// Serialize schema installation across replicas, using the database owner.
		_, err = tx.ExecContext(ctx, `SELECT pg_advisory_xact_lock(29001);
CREATE TABLE IF NOT EXISTS backend_profiles (
 player_id text PRIMARY KEY CHECK (player_id ~ '^[1-9][0-9]{0,19}$' AND player_id::numeric <= 18446744073709551615),
 display_name text NOT NULL DEFAULT 'Player' CHECK (octet_length(display_name) BETWEEN 1 AND 64),
 loadout jsonb
);
CREATE TABLE IF NOT EXISTS backend_sessions (
 token_hash text PRIMARY KEY CHECK (length(token_hash)=64),
 exchange_hash text NOT NULL UNIQUE CHECK (length(exchange_hash)=64),
 player_id text NOT NULL REFERENCES backend_profiles(player_id),
 expires_at timestamptz NOT NULL
);
CREATE INDEX IF NOT EXISTS backend_sessions_expiry ON backend_sessions(expires_at);`)
		if err == nil {
			err = tx.Commit()
		}
	}
	if err != nil {
		db.Close()
		return nil, err
	}
	return db, nil
}

type backendService struct {
	db                        *sql.DB
	steamURL, steamKey, appID string
	client                    *http.Client
	catalog                   map[string]bool
}

func backendJSON(w http.ResponseWriter, status int, value any) {
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Cache-Control", "no-store")
	w.WriteHeader(status)
	_ = json.NewEncoder(w).Encode(value)
}
func backendError(w http.ResponseWriter, status int, code string) {
	backendJSON(w, status, map[string]any{"version": 1, "error": code})
}
func backendDecode(r *http.Request, out any) error {
	data, err := io.ReadAll(io.LimitReader(r.Body, 8193))
	if err != nil {
		return err
	}
	if len(data) > 8192 {
		return errors.New("request too large")
	}
	return contracts.Decode(data, out)
}
func digest(data []byte) string {
	sum := sha256.Sum256(data)
	return hex.EncodeToString(sum[:])
}

var errBackendAuth = errors.New("authentication rejected")

func (b *backendService) steamIdentity(ctx context.Context, ticket string) (string, error) {
	// AuthenticateUserTicket is a publisher-only GET API. Never log its URL/errors.
	endpoint, err := url.Parse(b.steamURL)
	if err != nil || endpoint.Scheme != "https" || endpoint.Host == "" || b.client == nil || b.steamKey == "" || b.appID == "" {
		return "", errors.New("authentication unavailable")
	}
	query := endpoint.Query()
	query.Set("key", b.steamKey)
	query.Set("appid", b.appID)
	query.Set("ticket", ticket)
	query.Set("identity", "aftershock")
	endpoint.RawQuery = query.Encode()
	request, err := http.NewRequestWithContext(ctx, http.MethodGet, endpoint.String(), nil)
	if err != nil {
		return "", errors.New("authentication unavailable")
	}
	client := *b.client
	client.CheckRedirect = func(*http.Request, []*http.Request) error { return http.ErrUseLastResponse }
	response, err := client.Do(request)
	if err != nil {
		return "", errors.New("authentication unavailable")
	}
	defer response.Body.Close()
	if response.StatusCode >= 400 && response.StatusCode < 500 {
		return "", errBackendAuth
	}
	if response.StatusCode != http.StatusOK {
		return "", errors.New("authentication unavailable")
	}
	data, err := io.ReadAll(io.LimitReader(response.Body, 8193))
	if err != nil || len(data) > 8192 {
		return "", errors.New("authentication unavailable")
	}
	var result struct {
		Response struct {
			Params struct {
				Result, SteamID string
				PublisherBanned bool
			}
		}
	}
	if json.Unmarshal(data, &result) != nil || result.Response.Params.Result != "OK" ||
		!contracts.PlayerID(result.Response.Params.SteamID) || result.Response.Params.PublisherBanned {
		return "", errBackendAuth
	}
	return result.Response.Params.SteamID, nil
}
func (b *backendService) login(w http.ResponseWriter, r *http.Request) {
	var input struct {
		Ticket string `json:"ticket"`
	}
	if backendDecode(r, &input) != nil || len(input.Ticket) < 2 || len(input.Ticket) > 4096 {
		backendError(w, 400, "invalid_ticket")
		return
	}
	binary, err := hex.DecodeString(input.Ticket)
	if err != nil {
		backendError(w, 400, "invalid_ticket")
		return
	}
	player, err := b.steamIdentity(r.Context(), hex.EncodeToString(binary))
	if err != nil {
		if errors.Is(err, errBackendAuth) {
			backendError(w, 401, "authentication_rejected")
		} else {
			backendError(w, 503, "authentication_unavailable")
		}
		return
	}
	var random [32]byte
	if _, err := rand.Read(random[:]); err != nil {
		backendError(w, 503, "unavailable")
		return
	}
	token := hex.EncodeToString(random[:])
	expires := time.Now().Add(time.Hour)
	tx, err := b.db.BeginTx(r.Context(), nil)
	if err != nil {
		backendError(w, 503, "unavailable")
		return
	}
	defer tx.Rollback()
	_, err = tx.ExecContext(r.Context(), "INSERT INTO backend_profiles(player_id) VALUES($1) ON CONFLICT DO NOTHING", player)
	var stored string
	if err == nil {
		err = tx.QueryRowContext(r.Context(), `INSERT INTO backend_sessions(token_hash,exchange_hash,player_id,expires_at)
VALUES($1,$2,$3,$4) ON CONFLICT(exchange_hash) DO NOTHING RETURNING token_hash`, digest([]byte(token)), digest(binary), player, expires).Scan(&stored)
	}
	if errors.Is(err, sql.ErrNoRows) {
		backendError(w, 401, "authentication_rejected")
		return
	}
	if err == nil {
		err = tx.Commit()
	}
	if err != nil {
		backendError(w, 503, "unavailable")
		return
	}
	backendJSON(w, 200, map[string]any{"version": 1, "player_id": player, "token": token, "expires": expires.Unix()})
}
func (b *backendService) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	ctx, cancel := context.WithTimeout(r.Context(), 5*time.Second)
	defer cancel()
	r = r.WithContext(ctx)
	if r.URL.Path == "/v1/login" && r.Method == "POST" {
		b.login(w, r)
		return
	}
	token := strings.TrimPrefix(r.Header.Get("Authorization"), "Bearer ")
	if !strings.HasPrefix(r.Header.Get("Authorization"), "Bearer ") || len(token) != 64 {
		backendError(w, 401, "authentication_required")
		return
	}
	var player string
	err := b.db.QueryRowContext(ctx, "SELECT player_id FROM backend_sessions WHERE token_hash=$1 AND expires_at>CURRENT_TIMESTAMP", digest([]byte(token))).Scan(&player)
	if errors.Is(err, sql.ErrNoRows) {
		backendError(w, 401, "authentication_required")
		return
	}
	if err != nil {
		backendError(w, 503, "unavailable")
		return
	}
	switch {
	case r.URL.Path == "/v1/logout" && r.Method == "POST":
		// ponytail: retain one row per exchanged ticket; add cleanup when #180 supplies
		// a provider-proven ticket lifetime. Logout/expiry must not reopen old tickets.
		_, err = b.db.ExecContext(ctx, "UPDATE backend_sessions SET expires_at=CURRENT_TIMESTAMP WHERE token_hash=$1", digest([]byte(token)))
		if err != nil {
			backendError(w, 503, "unavailable")
			return
		}
		backendJSON(w, 200, map[string]any{"version": 1, "logged_out": true})
	case r.URL.Path == "/v1/profile" && r.Method == "PUT":
		var input struct {
			DisplayName string            `json:"display_name"`
			Loadout     contracts.Loadout `json:"loadout"`
		}
		if backendDecode(r, &input) != nil || len(input.DisplayName) < 1 || len(input.DisplayName) > 64 || !utf8.ValidString(input.DisplayName) || strings.IndexFunc(input.DisplayName, unicode.IsControl) >= 0 || input.Loadout.Validate(b.catalog) != nil {
			backendError(w, 400, "invalid_profile")
			return
		}
		data, err := json.Marshal(input.Loadout)
		if err == nil {
			_, err = b.db.ExecContext(ctx, "UPDATE backend_profiles SET display_name=$1,loadout=$2 WHERE player_id=$3", input.DisplayName, data, player)
		}
		if err != nil {
			backendError(w, 503, "unavailable")
			return
		}
		backendJSON(w, 200, map[string]any{"version": 1, "saved": true})
	case r.URL.Path == "/v1/profile" && r.Method == "GET":
		var name string
		var loadout []byte
		err = b.db.QueryRowContext(ctx, "SELECT display_name,loadout FROM backend_profiles WHERE player_id=$1", player).Scan(&name, &loadout)
		if err != nil {
			backendError(w, 503, "unavailable")
			return
		}
		backendJSON(w, 200, map[string]any{"version": 1, "player_id": player, "display_name": name, "loadout": json.RawMessage(loadout)})
	default:
		backendError(w, 404, "not_found")
	}
}
