//go:build ingest_integration

package main

import (
	"context"
	"crypto/sha256"
	"encoding/json"
	"errors"
	"os"
	"strings"
	"sync"
	"sync/atomic"
	"testing"

	"github.com/msetaro/aftershock/tools/match/contracts"
	"google.golang.org/grpc/metadata"
	"google.golang.org/protobuf/types/known/structpb"
)

// Draft: these tests run against an owned PostgreSQL database, never mock SQL.
func TestIngestCommitRetryAndRestart(t *testing.T) {
	ctx := context.Background()
	dsn := os.Getenv("INGEST_TEST_DATABASE")
	if dsn == "" {
		t.Fatal("run tests/ingest_services.py with its private PostgreSQL database")
	}
	db, err := openResultsDB(ctx, dsn)
	if err != nil {
		t.Fatal(err)
	}
	defer db.Close()
	var verifications atomic.Int32
	verify := func(_ context.Context, id, token string) ([]string, error) {
		verifications.Add(1)
		if id != "transaction-match" || token != "private-allocation-token" {
			return nil, errors.New("invalid allocation")
		}
		return []string{"18446744073709551615"}, nil
	}
	s := &durableIngest{db: db, verify: verify, reader: strings.Repeat("r", 32)}
	first := batch{Version: 1, Match: "transaction-match"}
	for _, line := range []string{"  0:00 InitGame: \\mapname\\two_lane", "  0:01 ClientIdentity: 0 18446744073709551615", "  0:01 ClientBegin: 0"} {
		first.Events = append(first.Events, line)
		first.End += int64(len(line) + 1)
		first.Checkpoint.add(line)
	}
	wire, err := matchBatch(first)
	if err != nil {
		t.Fatal(err)
	}
	count := func(table string) int {
		t.Helper()
		var n int
		if err := db.QueryRowContext(ctx, "SELECT count(*) FROM results."+table+" WHERE match_id=$1", first.Match).Scan(&n); err != nil {
			t.Fatal(err)
		}
		return n
	}
	if err := s.submit(ctx, wire, "wrong-token"); err == nil {
		t.Fatal("unauthenticated batch acknowledged")
	}
	// A real database failure in the middle of persistence must roll back every
	// preceding statement and must never acknowledge the batch.
	_, err = db.ExecContext(ctx, `CREATE FUNCTION results.reject_test_event() RETURNS trigger LANGUAGE plpgsql AS $$ BEGIN RAISE EXCEPTION 'owned rollback test'; END $$;
CREATE TRIGGER reject_test_event BEFORE INSERT ON results.events FOR EACH ROW EXECUTE FUNCTION results.reject_test_event();`)
	if err != nil {
		t.Fatal(err)
	}
	if err := s.submit(ctx, wire, "private-allocation-token"); err == nil {
		t.Fatal("failed transaction acknowledged")
	}
	if count("matches") != 0 || count("events") != 0 || count("batches") != 0 {
		t.Fatal("partial transaction became visible")
	}
	if _, err := db.ExecContext(ctx, "DROP TRIGGER reject_test_event ON results.events; DROP FUNCTION results.reject_test_event()"); err != nil {
		t.Fatal(err)
	}
	if err := s.submit(ctx, wire, "private-allocation-token"); err != nil {
		t.Fatal(err)
	}
	var group sync.WaitGroup
	for range 16 {
		group.Add(1)
		go func() {
			defer group.Done()
			if err := s.submit(ctx, wire, "private-allocation-token"); err != nil {
				t.Error(err)
			}
		}()
	}
	group.Wait()
	if count("events") != 3 || count("batches") != 1 {
		t.Fatal("retry duplicated committed data")
	}
	var tokenHash []byte
	if err := db.QueryRowContext(ctx, "SELECT token_hash FROM results.matches WHERE match_id=$1", first.Match).Scan(&tokenHash); err != nil {
		t.Fatal(err)
	}
	wantHash := sha256.Sum256([]byte("private-allocation-token"))
	if string(tokenHash) != string(wantHash[:]) {
		t.Fatal("allocation credential was not stored as a hash")
	}
	// Simulate a new stateless service instance after the GameServer is gone.
	// Existing durable credentials must authorize exact retries without Agones.
	s = &durableIngest{db: db, reader: strings.Repeat("r", 32), verify: func(context.Context, string, string) ([]string, error) {
		t.Error("retry consulted deleted GameServer")
		return nil, errors.New("gone")
	}}
	if err := s.submit(ctx, wire, "private-allocation-token"); err != nil {
		t.Fatal(err)
	}
	if err := s.submit(ctx, wire, "wrong-token"); err == nil {
		t.Fatal("stored match credential bypassed")
	}
	last := batch{Version: 1, Match: first.Match, Start: first.End, End: first.End, Checkpoint: first.Checkpoint, Final: true}
	for _, line := range []string{"  1:00 Kill: 1022 0 22: world killed player", "  1:00 Exit: Timelimit hit.", "  1:00 score: -1 ping: 0 client: 0 player"} {
		last.Events = append(last.Events, line)
		last.End += int64(len(line) + 1)
		last.Checkpoint.add(line)
	}
	final, err := matchBatch(last)
	if err != nil {
		t.Fatal(err)
	}
	clone := func(b contracts.MatchBatch) contracts.MatchBatch {
		data, _ := json.Marshal(b)
		var result contracts.MatchBatch
		if err := json.Unmarshal(data, &result); err != nil {
			t.Fatal(err)
		}
		return result
	}
	tampered := clone(final)
	tampered.Checkpoint.State.Seconds++
	if err := s.submit(ctx, tampered, "private-allocation-token"); err == nil {
		t.Fatal("checkpoint inconsistent with event stream acknowledged")
	}
	gap := clone(final)
	gap.Sequence++
	gap.End++
	gap.Checkpoint.Sequence++
	for i := range gap.Events {
		gap.Events[i].Sequence++
	}
	if err := s.submit(ctx, gap, "private-allocation-token"); err == nil {
		t.Fatal("gap in stream acknowledged")
	}
	var previous checkpoint
	for _, line := range first.Events {
		previous.add(line)
	}
	line := "  0:02 ClientIdentity: 1 2"
	unowned := batch{Version: 1, Match: first.Match, Start: first.End, End: first.End + int64(len(line)+1),
		Events: []string{line}, Checkpoint: previous}
	unowned.Checkpoint.add(line)
	unownedWire, err := matchBatch(unowned)
	if err != nil {
		t.Fatal(err)
	}
	if err := s.submit(ctx, unownedWire, "private-allocation-token"); err == nil {
		t.Fatal("account absent from verified allocation accepted")
	}
	if count("events") != 3 || count("batches") != 1 || count("player_results") != 0 {
		t.Fatal("rejected request changed durable state")
	}
	for range 3 {
		if err := s.submit(ctx, final, "private-allocation-token"); err != nil {
			t.Fatal(err)
		}
	}
	if count("events") != 6 || count("batches") != 2 || count("player_results") != 1 {
		t.Fatal("final retry duplicated events or results")
	}
	var score, deaths, kills int
	if err := db.QueryRowContext(ctx, "SELECT score,deaths,kills FROM results.player_results WHERE match_id=$1 AND player_id=$2", first.Match, "18446744073709551615").Scan(&score, &deaths, &kills); err != nil {
		t.Fatal(err)
	}
	if score != -1 || deaths != 1 || kills != 0 {
		t.Fatal("committed account totals differ from native log")
	}
	read := func(player, credential string) (resultsResponse, error) {
		request, _ := structpb.NewStruct(map[string]any{"version": 1, "player_id": player, "active_match": first.Match})
		response, err := s.Read(metadata.NewIncomingContext(ctx, metadata.Pairs("authorization", "Bearer "+credential)), request)
		var result resultsResponse
		if err == nil {
			data, marshalErr := response.MarshalJSON()
			if marshalErr != nil {
				t.Fatal(marshalErr)
			}
			err = json.Unmarshal(data, &result)
		}
		return result, err
	}
	result, err := read("18446744073709551615", strings.Repeat("r", 32))
	if err != nil || result.Finished != first.Match || len(result.Results) != 1 || len(result.Leaderboard) != 1 {
		t.Fatal("committed result not visible through reader API", err)
	}
	if result.Results[0].Stats.Score != -1 || result.Results[0].Stats.Deaths != 1 || result.Leaderboard[0].Matches != 1 {
		t.Fatal("reader returned duplicated or incorrect totals")
	}
	other, err := read("2", strings.Repeat("r", 32))
	if err != nil || len(other.Results) != 0 {
		t.Fatal("reader returned another account's owned result")
	}
	if _, err := read("18446744073709551615", "private-allocation-token"); err == nil {
		t.Fatal("allocation credential authorized reader RPC")
	}
	// Same sequence with a different payload is a conflict, not another event.
	changedData, _ := json.Marshal(final)
	var changed contracts.MatchBatch
	_ = json.Unmarshal(changedData, &changed)
	changed.Checkpoint.State.Seconds++
	if err := s.submit(ctx, changed, "private-allocation-token"); err == nil {
		t.Fatal("conflicting retry acknowledged")
	}
}
