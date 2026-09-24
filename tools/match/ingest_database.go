package main

import (
	"bytes"
	"context"
	"crypto/sha256"
	"crypto/subtle"
	"database/sql"
	"encoding/json"
	"errors"
	"sort"
	"strings"
	"time"

	"github.com/msetaro/aftershock/tools/match/contracts"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/emptypb"
	"google.golang.org/protobuf/types/known/structpb"
)

func openResultsDB(ctx context.Context, dsn string) (*sql.DB, error) {
	if dsn == "" {
		return nil, errors.New("results database configuration required")
	}
	db, err := sql.Open("pgx", dsn)
	if err != nil {
		return nil, errors.New("invalid results database configuration")
	}
	db.SetMaxOpenConns(16)
	db.SetMaxIdleConns(4)
	db.SetConnMaxLifetime(30 * time.Minute)
	tx, err := db.BeginTx(ctx, nil)
	if err == nil {
		defer tx.Rollback()
		_, err = tx.ExecContext(ctx, `SELECT pg_advisory_xact_lock(20260930);
CREATE SCHEMA IF NOT EXISTS results;
CREATE TABLE IF NOT EXISTS results.matches (
 match_id text PRIMARY KEY, token_hash bytea NOT NULL CHECK(octet_length(token_hash)=32),
 players jsonb NOT NULL, offset_bytes bigint NOT NULL DEFAULT 0, checkpoint jsonb NOT NULL,
 final boolean NOT NULL DEFAULT false
);
CREATE TABLE IF NOT EXISTS results.batches (
 match_id text NOT NULL REFERENCES results.matches(match_id), sequence bigint NOT NULL,
 final boolean NOT NULL, end_bytes bigint NOT NULL, digest bytea NOT NULL CHECK(octet_length(digest)=32),
 PRIMARY KEY(match_id,sequence,final)
);
CREATE TABLE IF NOT EXISTS results.events (
 match_id text NOT NULL REFERENCES results.matches(match_id), sequence bigint NOT NULL,
 version integer NOT NULL CHECK(version=1), timestamp integer NOT NULL, type text NOT NULL,
 raw bytea NOT NULL, PRIMARY KEY(match_id,sequence)
);
CREATE TABLE IF NOT EXISTS results.player_results (
 match_id text NOT NULL REFERENCES results.matches(match_id), player_id text NOT NULL,
 seconds integer NOT NULL, kills bigint NOT NULL, deaths bigint NOT NULL, score bigint NOT NULL,
 PRIMARY KEY(match_id,player_id)
);
CREATE INDEX IF NOT EXISTS results_player_page ON results.player_results(player_id,match_id DESC);
CREATE TABLE IF NOT EXISTS results.leaderboard (
 player_id text PRIMARY KEY, matches bigint NOT NULL, kills bigint NOT NULL,
 deaths bigint NOT NULL, score bigint NOT NULL
);
CREATE INDEX IF NOT EXISTS results_leaderboard_rank ON results.leaderboard(score DESC,player_id);`)
		if err == nil {
			err = tx.Commit()
		}
	}
	if err != nil {
		db.Close()
		return nil, errors.New("results database initialization failed")
	}
	return db, nil
}

type durableIngest struct {
	db     *sql.DB
	reader string
	verify func(context.Context, string, string) ([]string, error)
}

func ingestUnavailable() error {
	return status.Error(codes.Unavailable, "results persistence unavailable")
}

func (s *durableIngest) submit(ctx context.Context, b contracts.MatchBatch, token string) error {
	if !secret.MatchString(token) {
		return status.Error(codes.Unauthenticated, "allocation credential required")
	}
	if err := b.Validate(); err != nil {
		return status.Error(codes.InvalidArgument, "invalid match batch")
	}
	data, err := json.Marshal(b)
	if err != nil || len(data) > 65536 {
		return status.Error(codes.InvalidArgument, "batch exceeds bound")
	}
	digest, credential := sha256.Sum256(data), sha256.Sum256([]byte(token))
	var existing []byte
	err = s.db.QueryRowContext(ctx, "SELECT token_hash FROM results.matches WHERE match_id=$1", b.MatchID).Scan(&existing)
	newMatch := errors.Is(err, sql.ErrNoRows)
	var players []string
	if newMatch {
		if s.verify == nil {
			return ingestUnavailable()
		}
		players, err = s.verify(ctx, b.MatchID, token)
		if err != nil {
			return status.Error(codes.Unauthenticated, "allocated match identity required")
		}
	} else if err != nil {
		return ingestUnavailable()
	} else if subtle.ConstantTimeCompare(existing, credential[:]) != 1 {
		return status.Error(codes.Unauthenticated, "invalid allocation credential")
	}
	tx, err := s.db.BeginTx(ctx, nil)
	if err != nil {
		return ingestUnavailable()
	}
	defer tx.Rollback()
	if newMatch {
		allowed, _ := json.Marshal(players)
		initial, _ := json.Marshal(checkpoint{})
		_, err = tx.ExecContext(ctx, "INSERT INTO results.matches(match_id,token_hash,players,checkpoint) VALUES($1,$2,$3,$4) ON CONFLICT DO NOTHING", b.MatchID, credential[:], allowed, initial)
		if err != nil {
			return ingestUnavailable()
		}
	}
	var offset int64
	var final bool
	var saved, allowed []byte
	err = tx.QueryRowContext(ctx, "SELECT token_hash,players,offset_bytes,checkpoint,final FROM results.matches WHERE match_id=$1 FOR UPDATE", b.MatchID).Scan(&existing, &allowed, &offset, &saved, &final)
	if err != nil {
		return ingestUnavailable()
	}
	if subtle.ConstantTimeCompare(existing, credential[:]) != 1 {
		return status.Error(codes.Unauthenticated, "invalid allocation credential")
	}
	var previous []byte
	err = tx.QueryRowContext(ctx, "SELECT digest FROM results.batches WHERE match_id=$1 AND sequence=$2 AND final=$3", b.MatchID, b.Sequence, b.Final).Scan(&previous)
	if err == nil {
		if !bytes.Equal(previous, digest[:]) {
			return status.Error(codes.AlreadyExists, "conflicting batch retry")
		}
		// The digest is visible only after the original transaction committed.
		return nil
	}
	if !errors.Is(err, sql.ErrNoRows) {
		return ingestUnavailable()
	}
	if final || offset != b.Sequence {
		return status.Error(codes.FailedPrecondition, "closed or noncontiguous match stream")
	}
	var state checkpoint
	if json.Unmarshal(saved, &state) != nil || json.Unmarshal(allowed, &players) != nil || len(players) > 64 {
		return ingestUnavailable()
	}
	owners := make(map[string]bool, len(players))
	for _, player := range players {
		if !contracts.PlayerID(player) || owners[player] {
			return ingestUnavailable()
		}
		owners[player] = true
	}
	for _, event := range b.Events {
		raw, _ := event.Raw()
		state.add(raw)
	}
	for player := range state.Accounts {
		if !owners[player] {
			return status.Error(codes.PermissionDenied, "event owner absent from allocation")
		}
	}
	derived, _ := json.Marshal(state)
	claimed, _ := json.Marshal(b.Checkpoint.State)
	if !bytes.Equal(derived, claimed) {
		return status.Error(codes.InvalidArgument, "checkpoint disagrees with committed events")
	}
	for _, event := range b.Events {
		raw, _ := event.Raw()
		_, err = tx.ExecContext(ctx, "INSERT INTO results.events(match_id,sequence,version,timestamp,type,raw) VALUES($1,$2,$3,$4,$5,$6)", b.MatchID, event.Sequence, event.Version, event.Timestamp, event.Type, []byte(raw))
		if err != nil {
			return ingestUnavailable()
		}
	}
	if _, err = tx.ExecContext(ctx, "INSERT INTO results.batches(match_id,sequence,final,end_bytes,digest) VALUES($1,$2,$3,$4,$5)", b.MatchID, b.Sequence, b.Final, b.End, digest[:]); err != nil {
		return ingestUnavailable()
	}
	if _, err = tx.ExecContext(ctx, "UPDATE results.matches SET offset_bytes=$2,checkpoint=$3,final=$4 WHERE match_id=$1", b.MatchID, b.End, derived, b.Final); err != nil {
		return ingestUnavailable()
	}
	if b.Final && state.Completed {
		// A consistent account order avoids cross-match aggregate deadlocks.
		keys := make([]string, 0, len(state.Accounts))
		for player := range state.Accounts {
			keys = append(keys, player)
		}
		sort.Strings(keys)
		for _, player := range keys {
			stats := state.Accounts[player]
			_, err = tx.ExecContext(ctx, "INSERT INTO results.player_results(match_id,player_id,seconds,kills,deaths,score) VALUES($1,$2,$3,$4,$5,$6)", b.MatchID, player, state.Seconds, stats.Kills, stats.Deaths, stats.Score)
			if err == nil {
				_, err = tx.ExecContext(ctx, `INSERT INTO results.leaderboard(player_id,matches,kills,deaths,score) VALUES($1,1,$2,$3,$4)
ON CONFLICT(player_id) DO UPDATE SET matches=results.leaderboard.matches+1,kills=results.leaderboard.kills+excluded.kills,
deaths=results.leaderboard.deaths+excluded.deaths,score=results.leaderboard.score+excluded.score`, player, stats.Kills, stats.Deaths, stats.Score)
			}
			if err != nil {
				return ingestUnavailable()
			}
		}
	}
	if tx.Commit() != nil {
		return ingestUnavailable()
	}
	return nil
}

func rpcCredential(ctx context.Context) string {
	md, _ := metadata.FromIncomingContext(ctx)
	auth := md.Get("authorization")
	if len(auth) != 1 || len(auth[0]) > 263 || !strings.HasPrefix(auth[0], "Bearer ") {
		return ""
	}
	return strings.TrimPrefix(auth[0], "Bearer ")
}
func (s *durableIngest) Append(context.Context, *structpb.Struct) (*emptypb.Empty, error) {
	return nil, status.Error(codes.Unimplemented, "production ingestion requires versioned SubmitBatch")
}
func (s *durableIngest) SubmitBatch(ctx context.Context, input *structpb.Struct) (*emptypb.Empty, error) {
	if input == nil {
		return nil, status.Error(codes.InvalidArgument, "batch required")
	}
	data, err := input.MarshalJSON()
	if err != nil {
		return nil, status.Error(codes.InvalidArgument, "invalid batch")
	}
	b, err := contracts.DecodeMatchBatch(data)
	if err != nil {
		return nil, status.Error(codes.InvalidArgument, "invalid batch schema")
	}
	if err = s.submit(ctx, b, rpcCredential(ctx)); err != nil {
		return nil, err
	}
	return &emptypb.Empty{}, nil
}
