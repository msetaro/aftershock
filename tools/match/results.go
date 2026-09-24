package main

import (
	"context"
	"crypto/subtle"
	"encoding/json"
	"sort"
	"strings"

	"github.com/msetaro/aftershock/tools/match/contracts"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/structpb"
)

type matchResult struct {
	Match   string       `json:"match"`
	Seconds int          `json:"seconds"`
	Stats   accountStats `json:"stats"`
}
type leaderboardRow struct {
	PlayerID string `json:"player_id"`
	Matches  int    `json:"matches"`
	accountStats
}
type resultsResponse struct {
	Finished    string           `json:"finished_match"`
	Version     int              `json:"version"`
	Results     []matchResult    `json:"results"`
	Next        string           `json:"next"`
	Leaderboard []leaderboardRow `json:"leaderboard"`
}

// Read is separate from allocation-token writes. The caller is the authenticated
// backend, which supplies its verified account ID, never a client-selected owner.
func (s *ingest) Read(ctx context.Context, input *structpb.Struct) (*structpb.Struct, error) {
	md, _ := metadata.FromIncomingContext(ctx)
	auth := md.Get("authorization")
	if len(s.reader) < 32 || len(auth) != 1 || !strings.HasPrefix(auth[0], "Bearer ") || subtle.ConstantTimeCompare([]byte(strings.TrimPrefix(auth[0], "Bearer ")), []byte(s.reader)) != 1 {
		return nil, status.Error(codes.Unauthenticated, "reader credential required")
	}
	data, err := input.MarshalJSON()
	var query struct {
		Version  int    `json:"version"`
		PlayerID string `json:"player_id"`
		Before   string `json:"before,omitempty"`
		Active   string `json:"active_match,omitempty"`
		Match    string `json:"match,omitempty"`
	}
	if err != nil || strictJSON(data, &query) != nil || query.Version != 1 || !contracts.PlayerID(query.PlayerID) || (query.Before != "" && !identifier.MatchString(query.Before)) || (query.Active != "" && !identifier.MatchString(query.Active)) || (query.Match != "" && !identifier.MatchString(query.Match)) {
		return nil, status.Error(codes.InvalidArgument, "invalid result query")
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	result := resultsResponse{Version: 1, Results: []matchResult{}, Leaderboard: []leaderboardRow{}}
	if query.Active != "" && s.closed[query.Active] {
		result.Finished = query.Active
	}
	leaders := map[string]leaderboardRow{}
	// ponytail: #28's development stub scans final records in memory; #30 replaces
	// this with indexed transactional results storage. Responses stay bounded.
	for id, data := range s.results {
		var c checkpoint
		if json.Unmarshal(data, &c) != nil {
			return nil, status.Error(codes.Internal, "invalid stored result")
		}
		if stats, ok := c.Accounts[query.PlayerID]; ok && (query.Before == "" || id < query.Before) && (query.Match == "" || id == query.Match) {
			result.Results = append(result.Results, matchResult{id, c.Seconds, stats})
		}
		for player, stats := range c.Accounts {
			row := leaders[player]
			row.PlayerID = player
			row.Matches++
			row.Kills += stats.Kills
			row.Deaths += stats.Deaths
			row.Score += stats.Score
			leaders[player] = row
		}
	}
	sort.Slice(result.Results, func(i, j int) bool { return result.Results[i].Match > result.Results[j].Match })
	if len(result.Results) > 20 {
		result.Results = result.Results[:20]
		result.Next = result.Results[19].Match
	}
	for _, row := range leaders {
		result.Leaderboard = append(result.Leaderboard, row)
	}
	sort.Slice(result.Leaderboard, func(i, j int) bool {
		a, b := result.Leaderboard[i], result.Leaderboard[j]
		if a.Score != b.Score {
			return a.Score > b.Score
		}
		return a.PlayerID < b.PlayerID
	})
	if len(result.Leaderboard) > 100 {
		result.Leaderboard = result.Leaderboard[:100]
	}
	data, err = json.Marshal(result)
	if err != nil {
		return nil, err
	}
	var out structpb.Struct
	if err = out.UnmarshalJSON(data); err != nil {
		return nil, err
	}
	return &out, nil
}

func (s *ingest) retainResult(b batch) {
	if b.Final && b.Checkpoint.Completed {
		data, _ := json.Marshal(b.Checkpoint)
		s.results[b.Match] = data
	}
}
