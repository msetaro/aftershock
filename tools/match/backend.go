package main

import (
	"context"
	"crypto/rand"
	"crypto/sha256"
	"crypto/tls"
	"crypto/x509"
	"database/sql"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"log/slog"
	"net/http"
	"net/url"
	"os"
	"regexp"
	"strconv"
	"strings"
	"sync/atomic"
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
CREATE INDEX IF NOT EXISTS backend_sessions_expiry ON backend_sessions(expires_at);
CREATE TABLE IF NOT EXISTS backend_parties (
 id text PRIMARY KEY CHECK (length(id)=32),
 leader text NOT NULL UNIQUE REFERENCES backend_profiles(player_id),
 invite_hash text NOT NULL UNIQUE CHECK (length(invite_hash)=64)
);
CREATE TABLE IF NOT EXISTS backend_party_members (
 player_id text PRIMARY KEY REFERENCES backend_profiles(player_id),
 party_id text NOT NULL REFERENCES backend_parties(id) ON DELETE CASCADE
);
CREATE INDEX IF NOT EXISTS backend_party_members_party ON backend_party_members(party_id);
CREATE TABLE IF NOT EXISTS backend_matches (
 id text PRIMARY KEY, leader text NOT NULL REFERENCES backend_profiles(player_id),
 spec jsonb NOT NULL, join_key text NOT NULL CHECK (length(join_key)=64),
 ingest_token text NOT NULL CHECK (length(ingest_token)=64),
 state text NOT NULL DEFAULT 'queued' CHECK (state IN ('queued','allocated','complete')),
 address text NOT NULL DEFAULT '', server_name text NOT NULL DEFAULT '',
 created_at timestamptz NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE IF NOT EXISTS backend_match_members (
 match_id text NOT NULL REFERENCES backend_matches(id),
 player_id text NOT NULL REFERENCES backend_profiles(player_id), active boolean NOT NULL DEFAULT true,
 PRIMARY KEY(match_id,player_id)
);
CREATE UNIQUE INDEX IF NOT EXISTS backend_match_members_active ON backend_match_members(player_id) WHERE active;`)
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
	kubeURL, kubeToken, namespace, fleet, mapName string
	matchMinutes                                  int
	db                                            *sql.DB
	steamURL, steamKey, appID                     string
	client                                        *http.Client
	catalog                                       map[string]bool
	logger                                        *slog.Logger
	metrics                                       [6]struct{ requests, errors, nanos atomic.Uint64 }
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
func (b *backendService) handle(w http.ResponseWriter, r *http.Request) {
	ctx, cancel := context.WithTimeout(r.Context(), 5*time.Second)
	defer cancel()
	r = r.WithContext(ctx)
	if r.Method == "GET" && r.URL.Path == "/healthz" {
		if b.db.PingContext(ctx) != nil {
			backendError(w, 503, "unavailable")
		} else {
			backendJSON(w, 200, map[string]bool{"ready": true})
		}
		return
	}
	if r.Method == "GET" && r.URL.Path == "/metrics" {
		w.Header().Set("Content-Type", "text/plain; version=0.0.4")
		for i, name := range backendMetricNames {
			fmt.Fprintf(w, "aftershock_backend_requests_total{service=%q} %d\naftershock_backend_errors_total{service=%q} %d\naftershock_backend_duration_seconds_sum{service=%q} %.9f\n",
				name, b.metrics[i].requests.Load(), name, b.metrics[i].errors.Load(), name, float64(b.metrics[i].nanos.Load())/1e9)
		}
		return
	}
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
	case r.URL.Path == "/v1/queue":
		b.queue(w, r, player)
	case r.URL.Path == "/v1/party":
		b.party(w, r, player)
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

var backendMetricNames = [...]string{"auth", "profile", "party", "queue", "results", "other"}

type backendResponse struct {
	http.ResponseWriter
	status int
}

func (w *backendResponse) WriteHeader(status int) {
	if w.status == 0 {
		w.status = status
		w.ResponseWriter.WriteHeader(status)
	}
}
func (w *backendResponse) Write(data []byte) (int, error) {
	if w.status == 0 {
		w.WriteHeader(http.StatusOK)
	}
	return w.ResponseWriter.Write(data)
}
func (b *backendService) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	index := 5
	switch r.URL.Path {
	case "/v1/login", "/v1/logout":
		index = 0
	case "/v1/profile":
		index = 1
	case "/v1/party":
		index = 2
	case "/v1/queue":
		index = 3
	case "/v1/results", "/v1/leaderboard":
		index = 4
	}
	start := time.Now()
	response := &backendResponse{ResponseWriter: w}
	b.handle(response, r)
	duration := time.Since(start)
	b.metrics[index].requests.Add(1)
	b.metrics[index].nanos.Add(uint64(duration.Nanoseconds()))
	if response.status >= 400 {
		b.metrics[index].errors.Add(1)
	}
	if b.logger != nil {
		b.logger.Info("request", "service", backendMetricNames[index], "status", response.status, "duration_ms", duration.Milliseconds())
	}
}
func serveBackend(ctx context.Context) error {
	if os.Getenv("BACKEND_TLS_CERT") == "" || os.Getenv("BACKEND_TLS_KEY") == "" {
		return errors.New("BACKEND_TLS_CERT and BACKEND_TLS_KEY are required")
	}
	appID := os.Getenv("BACKEND_APP_ID")
	id, err := strconv.ParseUint(appID, 10, 32)
	if err != nil || id == 0 || strconv.FormatUint(id, 10) != appID {
		return errors.New("a canonical BACKEND_APP_ID is required")
	}
	secret, err := os.Open(os.Getenv("BACKEND_STEAM_KEY_FILE"))
	if err != nil {
		return errors.New("BACKEND_STEAM_KEY_FILE is required")
	}
	key, err := io.ReadAll(io.LimitReader(secret, 4097))
	secret.Close()
	if err != nil || len(key) > 4096 || strings.TrimSpace(string(key)) == "" {
		return errors.New("invalid publisher key file")
	}
	var weapons []string
	if err := readJSON(os.Getenv("BACKEND_CATALOG"), &weapons); err != nil || len(weapons) < 1 || len(weapons) > 256 {
		return errors.New("BACKEND_CATALOG must contain 1..256 weapon paths")
	}
	catalog := make(map[string]bool, len(weapons))
	for _, name := range weapons {
		catalog[name] = true
	}
	for _, name := range weapons {
		if (contracts.Loadout{Version: 1, Primary: name, Secondary: name}).Validate(catalog) != nil {
			return errors.New("invalid weapon catalog")
		}
	}
	roots, err := x509.SystemCertPool()
	if err != nil {
		return errors.New("system trust roots unavailable")
	}
	if path := os.Getenv("BACKEND_CA_FILE"); path != "" {
		data, err := os.ReadFile(path)
		if err != nil || !roots.AppendCertsFromPEM(data) {
			return errors.New("invalid BACKEND_CA_FILE")
		}
	}
	namespace, kubeURL, kubeToken := os.Getenv("BACKEND_NAMESPACE"), "", ""
	fleet, mapName := env("BACKEND_FLEET", "aftershock"), env("BACKEND_MAP", "two_lane")
	minutes, err := strconv.Atoi(env("BACKEND_MATCH_MINUTES", "10"))
	if err != nil || minutes < 1 || minutes > 1440 || !identifier.MatchString(mapName) || len(mapName) > 48 {
		return errors.New("invalid match map/duration")
	}
	if namespace != "" {
		dns := regexp.MustCompile(`^[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?$`)
		if !dns.MatchString(namespace) || !dns.MatchString(fleet) {
			return errors.New("invalid cluster namespace/Fleet")
		}
		kubeURL = env("BACKEND_KUBE_URL", "https://kubernetes.default.svc")
		parsed, err := url.Parse(kubeURL)
		if err != nil || parsed.Scheme != "https" || parsed.Host == "" || parsed.User != nil || parsed.RawQuery != "" || parsed.Fragment != "" {
			return errors.New("invalid HTTPS cluster endpoint")
		}
		credential, err := os.Open(env("BACKEND_KUBE_TOKEN_FILE", "/var/run/secrets/kubernetes.io/serviceaccount/token"))
		if err != nil {
			return errors.New("cluster credential unavailable")
		}
		data, err := io.ReadAll(io.LimitReader(credential, 16385))
		credential.Close()
		if err != nil || len(data) > 16384 || strings.TrimSpace(string(data)) == "" {
			return errors.New("invalid cluster credential")
		}
		kubeToken = strings.TrimSpace(string(data))
		data, err = os.ReadFile(env("BACKEND_KUBE_CA_FILE", "/var/run/secrets/kubernetes.io/serviceaccount/ca.crt"))
		if err != nil || !roots.AppendCertsFromPEM(data) {
			return errors.New("cluster trust roots unavailable")
		}
	}
	transport := http.DefaultTransport.(*http.Transport).Clone()
	transport.TLSClientConfig = &tls.Config{MinVersion: tls.VersionTLS12, RootCAs: roots}
	defer transport.CloseIdleConnections()
	endpoint := env("BACKEND_STEAM_URL", "https://partner.steam-api.com/ISteamUserAuth/AuthenticateUserTicket/v1/")
	parsedURL, err := url.Parse(endpoint)
	if err != nil || parsedURL.Scheme != "https" || parsedURL.Host == "" || parsedURL.User != nil || parsedURL.RawQuery != "" || parsedURL.Fragment != "" {
		return errors.New("invalid HTTPS Steam endpoint")
	}
	startup, cancel := context.WithTimeout(ctx, 15*time.Second)
	db, err := openBackendDB(startup, os.Getenv("BACKEND_DATABASE"))
	cancel()
	if err != nil {
		return errors.New("backend database initialization failed")
	}
	defer db.Close()
	service := &backendService{kubeURL: kubeURL, kubeToken: kubeToken, namespace: namespace, fleet: fleet, mapName: mapName, matchMinutes: minutes,
		db: db, steamURL: endpoint, steamKey: strings.TrimSpace(string(key)), appID: appID, catalog: catalog,
		client: &http.Client{Transport: transport, Timeout: 5 * time.Second}, logger: slog.New(slog.NewJSONHandler(os.Stdout, nil))}
	server := &http.Server{Addr: env("BACKEND_LISTEN", ":8443"), Handler: service, ReadHeaderTimeout: 5 * time.Second,
		ReadTimeout: 10 * time.Second, WriteTimeout: 10 * time.Second, IdleTimeout: 30 * time.Second, MaxHeaderBytes: 8192, TLSConfig: &tls.Config{MinVersion: tls.VersionTLS12}}
	stopped, shutdown := make(chan struct{}), make(chan struct{})
	go func() {
		defer close(shutdown)
		select {
		case <-ctx.Done():
			deadline, cancel := context.WithTimeout(context.Background(), 5*time.Second)
			defer cancel()
			if server.Shutdown(deadline) != nil {
				server.Close()
			}
		case <-stopped:
		}
	}()
	err = server.ListenAndServeTLS(os.Getenv("BACKEND_TLS_CERT"), os.Getenv("BACKEND_TLS_KEY"))
	close(stopped)
	<-shutdown
	if errors.Is(err, http.ErrServerClosed) {
		return nil
	}
	return err
}

type backendParty struct {
	ID         string   `json:"id"`
	Leader     string   `json:"leader"`
	Members    []string `json:"members"`
	InviteCode string   `json:"invite_code,omitempty"`
}

func (b *backendService) readParty(ctx context.Context, player string) (*backendParty, error) {
	rows, err := b.db.QueryContext(ctx, `SELECT p.id,p.leader,m.player_id FROM backend_parties p
JOIN backend_party_members mine ON mine.party_id=p.id JOIN backend_party_members m ON m.party_id=p.id
WHERE mine.player_id=$1 ORDER BY m.player_id`, player)
	if err != nil {
		return nil, err
	}
	defer rows.Close()
	var party *backendParty
	for rows.Next() {
		if party == nil {
			party = &backendParty{}
		}
		var member string
		if err := rows.Scan(&party.ID, &party.Leader, &member); err != nil {
			return nil, err
		}
		party.Members = append(party.Members, member)
		if len(party.Members) > 64 {
			return nil, errors.New("party exceeds server capacity")
		}
	}
	return party, rows.Err()
}
func (b *backendService) party(w http.ResponseWriter, r *http.Request, player string) {
	ctx := r.Context()
	invite, created := "", ""
	if r.Method != "GET" {
		if r.Method != "POST" && r.Method != "PUT" && r.Method != "DELETE" {
			backendError(w, 405, "unsupported_method")
			return
		}
		var input struct {
			InviteCode string `json:"invite_code"`
		}
		if backendDecode(r, &input) != nil || (r.Method == "PUT" && (len(input.InviteCode) != 32 || strings.Trim(input.InviteCode, "0123456789abcdef") != "")) || (r.Method != "PUT" && input.InviteCode != "") {
			backendError(w, 400, "invalid_party")
			return
		}
		tx, err := b.db.BeginTx(ctx, nil)
		if err != nil {
			backendError(w, 503, "unavailable")
			return
		}
		defer tx.Rollback()
		// ponytail: one short membership transaction at a time across replicas;
		// shard this database lock by party if measured queue traffic requires it.
		if _, err = tx.ExecContext(ctx, "SELECT pg_advisory_xact_lock(29002)"); err != nil {
			backendError(w, 503, "unavailable")
			return
		}
		var assigned bool
		if err = tx.QueryRowContext(ctx, "SELECT EXISTS(SELECT 1 FROM backend_match_members WHERE player_id=$1 AND active)", player).Scan(&assigned); err != nil {
			backendError(w, 503, "unavailable")
			return
		}
		if assigned {
			backendError(w, 409, "match_assigned")
			return
		}
		var current string
		err = tx.QueryRowContext(ctx, "SELECT party_id FROM backend_party_members WHERE player_id=$1", player).Scan(&current)
		if err != nil && !errors.Is(err, sql.ErrNoRows) {
			backendError(w, 503, "unavailable")
			return
		}
		if r.Method != "DELETE" && current != "" {
			backendError(w, 409, "already_in_party")
			return
		}
		switch r.Method {
		case "POST":
			created, invite = randomID(), randomID()
			_, err = tx.ExecContext(ctx, "INSERT INTO backend_parties(id,leader,invite_hash) VALUES($1,$2,$3)", created, player, digest([]byte(invite)))
			if err == nil {
				_, err = tx.ExecContext(ctx, "INSERT INTO backend_party_members(player_id,party_id) VALUES($1,$2)", player, created)
			}
		case "PUT":
			var target string
			err = tx.QueryRowContext(ctx, "SELECT id FROM backend_parties WHERE invite_hash=$1", digest([]byte(input.InviteCode))).Scan(&target)
			if errors.Is(err, sql.ErrNoRows) {
				backendError(w, 403, "invalid_invite")
				return
			}
			if err == nil {
				err = tx.QueryRowContext(ctx, `SELECT EXISTS(SELECT 1 FROM backend_party_members p JOIN backend_match_members m ON m.player_id=p.player_id WHERE p.party_id=$1 AND m.active)`, target).Scan(&assigned)
			}
			if err == nil && assigned {
				backendError(w, 409, "match_assigned")
				return
			}
			var count int
			if err == nil {
				err = tx.QueryRowContext(ctx, "SELECT count(*) FROM backend_party_members WHERE party_id=$1", target).Scan(&count)
			}
			if err == nil && count >= 64 {
				backendError(w, 409, "party_full")
				return
			}
			if err == nil {
				_, err = tx.ExecContext(ctx, "INSERT INTO backend_party_members(player_id,party_id) VALUES($1,$2)", player, target)
			}
		case "DELETE":
			_, err = tx.ExecContext(ctx, "DELETE FROM backend_parties WHERE id=$1 AND leader=$2", current, player)
			if err == nil {
				_, err = tx.ExecContext(ctx, "DELETE FROM backend_party_members WHERE player_id=$1", player)
			}
		}
		if err == nil {
			err = tx.Commit()
		}
		if err != nil {
			backendError(w, 503, "unavailable")
			return
		}
	}
	party, err := b.readParty(ctx, player)
	if err != nil {
		backendError(w, 503, "unavailable")
		return
	}
	if party != nil && party.ID == created {
		party.InviteCode = invite
	}
	backendJSON(w, 200, map[string]any{"version": 1, "party": party})
}
