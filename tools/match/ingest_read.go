package main

import (
	"context"
	"crypto/subtle"
	"encoding/json"

	"github.com/msetaro/aftershock/tools/match/contracts"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/structpb"
)

// Only the authenticated backend holds the reader credential. This method never
// mutates match state; neither backend database credentials nor allocation tokens
// grant access to the results database or this reader API.
func (s *durableIngest) Read(ctx context.Context, input *structpb.Struct) (*structpb.Struct, error) {
	if len(s.reader) < 32 || subtle.ConstantTimeCompare([]byte(rpcCredential(ctx)), []byte(s.reader)) != 1 {
		return nil, status.Error(codes.Unauthenticated, "reader credential required")
	}
	if input == nil {
		return nil, status.Error(codes.InvalidArgument, "query required")
	}
	data, err := input.MarshalJSON()
	var query struct {
		Version  int    `json:"version"`
		PlayerID string `json:"player_id"`
		Before   string `json:"before,omitempty"`
		Active   string `json:"active_match,omitempty"`
		Match    string `json:"match,omitempty"`
	}
	if err != nil || strictJSON(data, &query) != nil || query.Version != 1 || !contracts.PlayerID(query.PlayerID) ||
		(query.Before != "" && !identifier.MatchString(query.Before)) || (query.Active != "" && !identifier.MatchString(query.Active)) || (query.Match != "" && !identifier.MatchString(query.Match)) {
		return nil, status.Error(codes.InvalidArgument, "invalid result query")
	}
	result := resultsResponse{Version: 1, Results: []matchResult{}, Leaderboard: []leaderboardRow{}}
	if query.Active != "" {
		var finished bool
		if err = s.db.QueryRowContext(ctx, "SELECT EXISTS(SELECT 1 FROM results.matches WHERE match_id=$1 AND final)", query.Active).Scan(&finished); err != nil {
			return nil, ingestUnavailable()
		}
		if finished {
			result.Finished = query.Active
		}
	}
	rows, err := s.db.QueryContext(ctx, `SELECT match_id,seconds,kills,deaths,score FROM results.player_results
WHERE player_id=$1 AND ($2='' OR match_id<$2) AND ($3='' OR match_id=$3) ORDER BY match_id DESC LIMIT 21`, query.PlayerID, query.Before, query.Match)
	if err != nil {
		return nil, ingestUnavailable()
	}
	for rows.Next() {
		var value matchResult
		if err = rows.Scan(&value.Match, &value.Seconds, &value.Stats.Kills, &value.Stats.Deaths, &value.Stats.Score); err != nil {
			rows.Close()
			return nil, ingestUnavailable()
		}
		result.Results = append(result.Results, value)
	}
	err = rows.Err()
	rows.Close()
	if err != nil {
		return nil, ingestUnavailable()
	}
	if len(result.Results) > 20 {
		result.Results = result.Results[:20]
		result.Next = result.Results[19].Match
	}
	rows, err = s.db.QueryContext(ctx, "SELECT player_id,matches,kills,deaths,score FROM results.leaderboard ORDER BY score DESC,player_id LIMIT 100")
	if err != nil {
		return nil, ingestUnavailable()
	}
	for rows.Next() {
		var value leaderboardRow
		if err = rows.Scan(&value.PlayerID, &value.Matches, &value.Kills, &value.Deaths, &value.Score); err != nil {
			rows.Close()
			return nil, ingestUnavailable()
		}
		result.Leaderboard = append(result.Leaderboard, value)
	}
	err = rows.Err()
	rows.Close()
	if err != nil {
		return nil, ingestUnavailable()
	}
	data, err = json.Marshal(result)
	if err != nil {
		return nil, ingestUnavailable()
	}
	var response structpb.Struct
	if response.UnmarshalJSON(data) != nil {
		return nil, ingestUnavailable()
	}
	return &response, nil
}
