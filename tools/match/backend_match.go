package main

import (
	"bytes"
	"context"
	"database/sql"
	"encoding/hex"
	"encoding/json"
	"errors"
	"io"
	"net"
	"net/http"
	"net/url"
	"strconv"
	"strings"
	"time"

	"github.com/msetaro/aftershock/tools/match/contracts"
	"google.golang.org/grpc"
	"google.golang.org/grpc/metadata"
	"google.golang.org/protobuf/types/known/structpb"
)

func (b *backendService) queue(w http.ResponseWriter, r *http.Request, player string) {
	if r.Method != "POST" && r.Method != "GET" {
		backendError(w, 405, "unsupported_method")
		return
	}
	if b.kubeURL == "" || b.kubeToken == "" || b.namespace == "" {
		backendError(w, 503, "matchmaking_unavailable")
		return
	}
	ctx := r.Context()
	var input struct {
		Map  string `json:"map"`
		Mode *int   `json:"mode"`
	}
	if r.Method == "POST" && (backendDecode(r, &input) != nil || input.Map != b.mapName || input.Mode == nil || *input.Mode < 0 || *input.Mode > 4) {
		backendError(w, 400, "invalid_match")
		return
	}
	tx, err := b.db.BeginTx(ctx, nil)
	if err != nil {
		backendError(w, 503, "unavailable")
		return
	}
	defer tx.Rollback()
	// The same short lock as party edits freezes expected-player membership.
	if r.Method == "POST" {
		_, err = tx.ExecContext(ctx, "SELECT pg_advisory_xact_lock(29002)")
	}
	if err != nil {
		backendError(w, 503, "unavailable")
		return
	}
	var id string
	err = tx.QueryRowContext(ctx, "SELECT match_id FROM backend_match_members WHERE player_id=$1 AND active", player).Scan(&id)
	if err != nil && !errors.Is(err, sql.ErrNoRows) {
		backendError(w, 503, "unavailable")
		return
	}
	if id == "" && r.Method == "GET" {
		backendJSON(w, 200, map[string]any{"version": 1, "state": "idle"})
		return
	}
	if id == "" {
		players := []string{player}
		var party, leader string
		err = tx.QueryRowContext(ctx, "SELECT p.id,p.leader FROM backend_parties p JOIN backend_party_members m ON m.party_id=p.id WHERE m.player_id=$1", player).Scan(&party, &leader)
		if err != nil && !errors.Is(err, sql.ErrNoRows) {
			backendError(w, 503, "unavailable")
			return
		}
		if party != "" {
			if leader != player {
				backendError(w, 403, "party_leader_required")
				return
			}
			rows, err := tx.QueryContext(ctx, "SELECT player_id FROM backend_party_members WHERE party_id=$1 ORDER BY player_id", party)
			if err != nil {
				backendError(w, 503, "unavailable")
				return
			}
			players = nil
			for rows.Next() {
				var member string
				if err = rows.Scan(&member); err != nil {
					break
				}
				players = append(players, member)
			}
			rowErr := rows.Err()
			rows.Close()
			if err != nil || rowErr != nil {
				backendError(w, 503, "unavailable")
				return
			}
		}
		members, _ := json.Marshal(players)
		var busy bool
		if err = tx.QueryRowContext(ctx, `SELECT EXISTS(SELECT 1 FROM backend_match_members WHERE active AND player_id IN (SELECT jsonb_array_elements_text($1::jsonb)))`, members).Scan(&busy); err != nil {
			backendError(w, 503, "unavailable")
			return
		}
		if busy {
			backendError(w, 409, "member_already_assigned")
			return
		}
		id = randomID()
		spec := contracts.MatchSpec{Version: 1, ID: id, Map: input.Map, Mode: *input.Mode, Slots: len(players), Rules: contracts.Rules{FragLimit: 20, TimeLimit: b.matchMinutes}, ExpectedPlayers: players}
		if spec.Validate() != nil {
			backendError(w, 400, "invalid_match")
			return
		}
		data, _ := json.Marshal(spec)
		_, err = tx.ExecContext(ctx, "INSERT INTO backend_matches(id,leader,spec,join_key,ingest_token) VALUES($1,$2,$3,$4,$5)", id, player, data, randomID()+randomID(), randomID()+randomID())
		if err == nil {
			_, err = tx.ExecContext(ctx, "INSERT INTO backend_match_members(match_id,player_id) SELECT $1,jsonb_array_elements_text($2::jsonb)", id, members)
		}
	} else if r.Method == "POST" {
		var same bool
		err = tx.QueryRowContext(ctx, "SELECT spec->>'map'=$2 AND (spec->>'mode')::int=$3 FROM backend_matches WHERE id=$1", id, input.Map, *input.Mode).Scan(&same)
		if err == nil && !same {
			backendError(w, 409, "match_already_assigned")
			return
		}
	}
	if err == nil {
		err = tx.Commit()
	}
	if err != nil {
		backendError(w, 503, "unavailable")
		return
	}
	// Membership is committed and the global lock released before contacting Agones.
	assignment, err := b.allocate(ctx, id)
	if err != nil {
		backendError(w, 503, "allocation_unavailable")
		return
	}
	if assignment.Address == "" {
		backendJSON(w, 202, map[string]any{"version": 1, "state": "queued", "match_id": id})
		return
	}
	if probeMatch(assignment.Address, assignment.Match.Map, id) != nil {
		backendJSON(w, 202, map[string]any{"version": 1, "state": "starting", "match_id": id})
		return
	}
	key, err := hex.DecodeString(assignment.JoinKey)
	now := time.Now().Unix()
	ticket, signErr := contracts.SignJoin(key, contracts.JoinTicket{Version: 1, PlayerID: player, MatchID: id, IssuedAt: now, ExpiresAt: now + 120, Nonce: randomID()})
	if err != nil || signErr != nil {
		backendError(w, 503, "unavailable")
		return
	}
	backendJSON(w, 200, map[string]any{"version": 1, "state": "allocated", "match_id": id, "address": assignment.Address, "ticket": ticket, "expires": now + 120})
}

type backendAssignment struct {
	Match                   contracts.MatchSpec
	JoinKey, Token, Address string
}
type backendGameStatus struct {
	State, Address, GameServerName string
	Ports                          []struct {
		Name string
		Port int
	}
}

func (b *backendService) kube(ctx context.Context, method, path string, body, out any) error {
	var reader io.Reader
	if body != nil {
		data, err := json.Marshal(body)
		if err != nil {
			return err
		}
		reader = bytes.NewReader(data)
	}
	request, err := http.NewRequestWithContext(ctx, method, strings.TrimSuffix(b.kubeURL, "/")+path, reader)
	if err != nil {
		return errors.New("cluster request unavailable")
	}
	request.Header.Set("Authorization", "Bearer "+b.kubeToken)
	request.Header.Set("Content-Type", "application/json")
	client := *b.client
	client.CheckRedirect = func(*http.Request, []*http.Request) error { return http.ErrUseLastResponse }
	response, err := client.Do(request)
	if err != nil {
		return errors.New("cluster request unavailable")
	}
	defer response.Body.Close()
	if response.StatusCode != 200 && response.StatusCode != 201 {
		return errors.New("cluster request rejected")
	}
	data, err := io.ReadAll(io.LimitReader(response.Body, 65537))
	if err != nil || len(data) > 65536 {
		return errors.New("invalid cluster response")
	}
	return json.Unmarshal(data, out)
}
func (b *backendService) allocate(ctx context.Context, id string) (backendAssignment, error) {
	var result backendAssignment
	tx, err := b.db.BeginTx(ctx, nil)
	if err != nil {
		return result, err
	}
	defer tx.Rollback()
	var encoded []byte
	var state string
	err = tx.QueryRowContext(ctx, "SELECT spec,join_key,ingest_token,address,state FROM backend_matches WHERE id=$1 FOR UPDATE", id).Scan(&encoded, &result.JoinKey, &result.Token, &result.Address, &state)
	if err != nil {
		return result, err
	}
	if json.Unmarshal(encoded, &result.Match) != nil || result.Match.Validate() != nil {
		return result, errors.New("invalid stored assignment")
	}
	if state == "allocated" {
		return result, nil
	}
	if state != "queued" {
		return result, errors.New("match is no longer joinable")
	}
	var servers struct {
		Items []struct {
			Metadata struct {
				Name   string
				Labels map[string]string
			}
			Status backendGameStatus
		}
	}
	path := "/apis/agones.dev/v1/namespaces/" + url.PathEscape(b.namespace) + "/gameservers?labelSelector=" + url.QueryEscape("aftershock.dev/match="+id)
	if err = b.kube(ctx, "GET", path, nil, &servers); err != nil {
		return result, err
	}
	if len(servers.Items) > 1 {
		return result, errors.New("duplicate match allocation")
	}
	var status backendGameStatus
	if len(servers.Items) == 1 {
		server := servers.Items[0]
		if server.Metadata.Labels["aftershock.dev/match"] != id {
			return result, errors.New("mismatched recovered allocation")
		}
		status = server.Status
		status.GameServerName = server.Metadata.Name
	} else {
		annotation, _ := json.Marshal(struct {
			Match   contracts.MatchSpec `json:"match"`
			JoinKey string              `json:"join_key"`
			Token   string              `json:"token"`
		}{result.Match, result.JoinKey, result.Token})
		allocation := map[string]any{"apiVersion": "allocation.agones.dev/v1", "kind": "GameServerAllocation", "metadata": map[string]string{"namespace": b.namespace},
			"spec": map[string]any{"selectors": []any{map[string]any{"matchLabels": map[string]string{"agones.dev/fleet": b.fleet}}},
				"metadata": map[string]any{"labels": map[string]string{"aftershock.dev/match": id}, "annotations": map[string]string{"aftershock.dev/match": string(annotation)}}}}
		var response struct{ Status backendGameStatus }
		if err = b.kube(ctx, "POST", "/apis/allocation.agones.dev/v1/namespaces/"+url.PathEscape(b.namespace)+"/gameserverallocations", allocation, &response); err != nil {
			return result, err
		}
		status = response.Status
		if status.State == "UnAllocated" {
			return result, nil
		}
	}
	if status.State != "Allocated" || !identifier.MatchString(status.GameServerName) || net.ParseIP(status.Address) == nil {
		return result, errors.New("invalid allocated server")
	}
	port := 0
	for _, candidate := range status.Ports {
		if candidate.Name == "game" {
			if port != 0 {
				return result, errors.New("duplicate game port")
			}
			port = candidate.Port
		}
	}
	if port < 1024 || port > 65535 {
		return result, errors.New("invalid game port")
	}
	result.Address = net.JoinHostPort(status.Address, strconv.Itoa(port))
	_, err = tx.ExecContext(ctx, "UPDATE backend_matches SET state='allocated',address=$2,server_name=$3 WHERE id=$1", id, result.Address, status.GameServerName)
	if err == nil {
		err = tx.Commit()
	}
	return result, err
}

func (b *backendService) readResults(w http.ResponseWriter, r *http.Request, player string) {
	if r.Method != "GET" {
		backendError(w, 405, "unsupported_method")
		return
	}
	query, err := url.ParseQuery(r.URL.RawQuery)
	if err != nil || len(query) > 1 || (len(query) == 1 && len(query["before"]) != 1) || (query.Get("before") != "" && !identifier.MatchString(query.Get("before"))) {
		backendError(w, 400, "invalid_query")
		return
	}
	if b.results == nil || b.reader == "" {
		backendError(w, 503, "results_unavailable")
		return
	}
	var active string
	err = b.db.QueryRowContext(r.Context(), "SELECT match_id FROM backend_match_members WHERE player_id=$1 AND active", player).Scan(&active)
	if err != nil && !errors.Is(err, sql.ErrNoRows) {
		backendError(w, 503, "unavailable")
		return
	}
	input, _ := structpb.NewStruct(map[string]any{"version": 1, "player_id": player, "before": query.Get("before"), "active_match": active})
	var response structpb.Struct
	ctx := metadata.AppendToOutgoingContext(r.Context(), "authorization", "Bearer "+b.reader)
	if err = b.results.Invoke(ctx, "/aftershock.match.v1.Ingest/Read", input, &response, grpc.MaxCallRecvMsgSize(65536)); err != nil {
		backendError(w, 503, "results_unavailable")
		return
	}
	data, err := response.MarshalJSON()
	var result resultsResponse
	if err != nil || strictJSON(data, &result) != nil || result.Version != 1 || len(result.Results) > 20 || len(result.Leaderboard) > 100 || (result.Finished != "" && result.Finished != active) {
		backendError(w, 503, "invalid_results")
		return
	}
	// Only queue ownership changes here. Match data belongs to the ingest service.
	if result.Finished != "" {
		tx, err := b.db.BeginTx(r.Context(), nil)
		if err != nil {
			backendError(w, 503, "unavailable")
			return
		}
		defer tx.Rollback()
		_, err = tx.ExecContext(r.Context(), "SELECT pg_advisory_xact_lock(29002)")
		if err == nil {
			_, err = tx.ExecContext(r.Context(), `WITH finished AS (
UPDATE backend_matches SET state='complete' WHERE id=$1 AND state='allocated'
AND EXISTS(SELECT 1 FROM backend_match_members WHERE match_id=$1 AND player_id=$2) RETURNING id)
UPDATE backend_match_members SET active=false WHERE match_id IN (SELECT id FROM finished)`, result.Finished, player)
		}
		if err == nil {
			err = tx.Commit()
		}
		if err != nil {
			backendError(w, 503, "unavailable")
			return
		}
	}
	if r.URL.Path == "/v1/leaderboard" {
		result.Results = []matchResult{}
		result.Next = ""
	}
	backendJSON(w, 200, result)
}
