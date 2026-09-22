package main

import (
	"context"
	"encoding/json"
	"fmt"
	"path/filepath"
	"strings"
	"testing"

	"google.golang.org/grpc/metadata"
	"google.golang.org/protobuf/types/known/structpb"
)

func TestReadAcknowledgedResults(t *testing.T) {
	path := filepath.Join(t.TempDir(), "events.jsonl")
	tokens := map[string]string{"result-1": "allocation-secret"}
	s, err := newIngest(path, tokens)
	if err != nil {
		t.Fatal(err)
	}
	s.reader = strings.Repeat("r", 64)
	input, _ := structpb.NewStruct(map[string]any{"version": 1, "player_id": "123"})
	ctx := metadata.NewIncomingContext(context.Background(), metadata.Pairs("authorization", "Bearer "+s.reader))
	if _, err = s.Read(context.Background(), input); err == nil {
		t.Fatal("unauthorized result read")
	}
	bad, _ := structpb.NewStruct(map[string]any{"version": 1, "player_id": "0123"})
	if _, err = s.Read(ctx, bad); err == nil {
		t.Fatal("invalid identity read")
	}
	var c checkpoint
	line := "0:01 ClientIdentity: 1 123"
	c.add(line)
	b := batch{Version: 1, Match: "result-1", End: int64(len(line) + 1), Events: []string{line}, Checkpoint: c}
	if err = s.accept(b, tokens[b.Match]); err != nil {
		t.Fatal(err)
	}
	read := func(player string) resultsResponse {
		t.Helper()
		query, _ := structpb.NewStruct(map[string]any{"version": 1, "player_id": player})
		out, err := s.Read(ctx, query)
		if err != nil {
			t.Fatal(err)
		}
		data, _ := out.MarshalJSON()
		if strings.Contains(string(data), "allocation-secret") || strings.Contains(string(data), "owners") {
			t.Fatal("private result fields leaked")
		}
		var result resultsResponse
		if err = strictJSON(data, &result); err != nil {
			t.Fatal(err)
		}
		return result
	}
	if r := read("123"); len(r.Results) != 0 || len(r.Leaderboard) != 0 {
		t.Fatal("unfinished match appeared", r)
	}
	c.add("0:02 ClientIdentity: 2 456")
	c.add("0:03 Kill: 1 2 7: a killed b")
	c.add("0:04 score: 5 ping: 1 client: 1 a")
	c.add("0:05 Exit: Timelimit hit.")
	b.Start = b.End
	b.Events = nil
	b.Final = true
	b.Checkpoint = c
	if err = s.accept(b, tokens[b.Match]); err != nil {
		t.Fatal(err)
	}
	if err = s.accept(b, tokens[b.Match]); err != nil {
		t.Fatal(err)
	}
	r := read("123")
	if len(r.Results) != 1 || r.Results[0].Stats.Score != 5 || len(r.Leaderboard) != 2 || r.Leaderboard[0].PlayerID != "123" || r.Leaderboard[0].Matches != 1 {
		t.Fatal(r)
	}
	if r := read("789"); len(r.Results) != 0 {
		t.Fatal("another player's private history returned", r)
	}
	s.file.Close()
	s, err = newIngest(path, tokens)
	if err != nil {
		t.Fatal(err)
	}
	defer s.file.Close()
	s.reader = strings.Repeat("r", 64)
	r = read("123")
	if len(r.Results) != 1 || r.Results[0].Stats.Kills != 1 {
		t.Fatal("lost acknowledged result after restart", r)
	}
	// Returned structures cannot mutate durable read state.
	data, _ := json.Marshal(r)
	if !strings.Contains(string(data), "result-1") {
		t.Fatal(string(data))
	}
}

func TestResultsBoundsAndCursor(t *testing.T) {
	s, err := newIngest(filepath.Join(t.TempDir(), "events.jsonl"), map[string]string{})
	if err != nil {
		t.Fatal(err)
	}
	defer s.file.Close()
	s.reader = strings.Repeat("r", 64)
	for i := 0; i < 105; i++ {
		id := fmt.Sprintf("match-%03d", i)
		s.tokens[id] = "allocation-secret"
		accounts := map[string]accountStats{"123": {Score: i}, fmt.Sprint(1000 + i): {Score: i}}
		b := batch{Version: 1, Match: id, Final: true, Checkpoint: checkpoint{Completed: true, Accounts: accounts}}
		if err = s.accept(b, s.tokens[id]); err != nil {
			t.Fatal(err)
		}
	}
	ctx := metadata.NewIncomingContext(context.Background(), metadata.Pairs("authorization", "Bearer "+s.reader))
	seen := map[string]bool{}
	before := ""
	for {
		query, _ := structpb.NewStruct(map[string]any{"version": 1, "player_id": "123", "before": before})
		response, err := s.Read(ctx, query)
		if err != nil {
			t.Fatal(err)
		}
		data, _ := response.MarshalJSON()
		var result resultsResponse
		if err = strictJSON(data, &result); err != nil {
			t.Fatal(err)
		}
		if len(result.Results) > 20 || len(result.Leaderboard) != 100 || len(data) > 32768 {
			t.Fatal("unbounded response")
		}
		for _, r := range result.Results {
			if seen[r.Match] {
				t.Fatal("duplicate page")
			}
			seen[r.Match] = true
		}
		if result.Next == "" {
			break
		}
		before = result.Next
	}
	query, _ := structpb.NewStruct(map[string]any{"version": 1, "player_id": "123", "match": "match-000"})
	response, err := s.Read(ctx, query)
	if err != nil {
		t.Fatal(err)
	}
	data, _ := response.MarshalJSON()
	var exact resultsResponse
	if err = strictJSON(data, &exact); err != nil {
		t.Fatal(err)
	}
	if len(exact.Results) != 1 || exact.Results[0].Match != "match-000" || exact.Next != "" {
		t.Fatal("exact match lost behind history page", exact)
	}
	if len(seen) != 105 {
		t.Fatal("missing history", len(seen))
	}
}
