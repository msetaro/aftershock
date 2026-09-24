package contracts

import (
	"encoding/base64"
	"encoding/json"
	"errors"
	"fmt"
	"regexp"
	"strconv"
	"strings"
)

const MaxMatchOffset int64 = 1 << 33 // Exact in the existing protobuf Struct transport.

var eventName = regexp.MustCompile(`^[A-Za-z][A-Za-z0-9_]{0,63}$`)

type MatchEvent struct {
	Version   int    `json:"version"`
	MatchID   string `json:"match_id"`
	Sequence  int64  `json:"sequence"`
	Timestamp int    `json:"timestamp"`
	Type      string `json:"type"`
	Payload   string `json:"payload"` // Canonical base64 of one newline-free native log record.
}

type PlayerStats struct {
	Kills  int `json:"kills"`
	Deaths int `json:"deaths"`
}
type AccountStats struct {
	Kills  int `json:"kills"`
	Deaths int `json:"deaths"`
	Score  int `json:"score"`
}
type MatchState struct {
	Accounts    map[string]AccountStats `json:"accounts,omitempty"`
	Owners      map[string]string       `json:"owners,omitempty"`
	OwnerScores map[string]int          `json:"owner_scores,omitempty"`
	Seconds     int                     `json:"seconds"`
	Players     map[string]PlayerStats  `json:"players"`
	Joins       int                     `json:"joins"`
	Kills       int                     `json:"kills"`
	Scores      map[string]int          `json:"scores"`
	Completed   bool                    `json:"completed"`
}
type MatchCheckpoint struct {
	Version  int        `json:"version"`
	MatchID  string     `json:"match_id"`
	Sequence int64      `json:"sequence"`
	State    MatchState `json:"state"`
}
type MatchBatch struct {
	Version    int             `json:"version"`
	MatchID    string          `json:"match_id"`
	Sequence   int64           `json:"sequence"`
	End        int64           `json:"end"`
	Events     []MatchEvent    `json:"events"`
	Checkpoint MatchCheckpoint `json:"checkpoint"`
	Final      bool            `json:"final"`
}

func slot(s string) bool {
	n, err := strconv.Atoi(s)
	return err == nil && n >= 0 && n < 64 && strconv.Itoa(n) == s
}
func signedStat(n int) bool { return n >= -2147483648 && n <= 2147483647 }
func countStat(n int) bool  { return n >= 0 && n <= 2147483647 }

func (s MatchState) Validate() error {
	invalid := errors.New("invalid checkpoint state")
	if !countStat(s.Seconds) || !countStat(s.Joins) || !countStat(s.Kills) || len(s.Accounts) > 64 || len(s.Owners) > 64 || len(s.OwnerScores) > 64 || len(s.Players) > 64 || len(s.Scores) > 64 {
		return invalid
	}
	for player, stats := range s.Accounts {
		if !PlayerID(player) || !countStat(stats.Kills) || !countStat(stats.Deaths) || !signedStat(stats.Score) {
			return invalid
		}
	}
	for key, player := range s.Owners {
		if _, ok := s.Accounts[player]; !ok || !slot(key) || !PlayerID(player) {
			return invalid
		}
	}
	for key, score := range s.OwnerScores {
		if !slot(key) || !signedStat(score) || s.Owners[key] == "" {
			return invalid
		}
	}
	for key, stats := range s.Players {
		if !slot(key) || !countStat(stats.Kills) || !countStat(stats.Deaths) {
			return invalid
		}
	}
	for key, score := range s.Scores {
		if !slot(key) || !signedStat(score) {
			return invalid
		}
	}
	return nil
}

func eventHeader(raw string) (int, string, error) {
	invalid := errors.New("invalid native event")
	if len(raw) > 4095 || strings.ContainsAny(raw, "\r\n") {
		return 0, "", invalid
	}
	fields := strings.Fields(raw)
	if len(fields) < 2 || !strings.Contains(fields[0], ":") {
		return 0, "raw", nil
	}
	clock := strings.Split(fields[0], ":")
	if len(clock) != 2 {
		return 0, "raw", nil
	}
	minutes, e1 := strconv.ParseInt(clock[0], 10, 32)
	seconds, e2 := strconv.ParseInt(clock[1], 10, 32)
	if e1 != nil || e2 != nil || minutes < 0 || seconds < 0 || seconds > 59 || minutes*60+seconds > 2147483647 {
		return 0, "", invalid
	}
	kind := strings.TrimSuffix(fields[1], ":")
	if !eventName.MatchString(kind) {
		kind = "raw"
	}
	var a, b, c, m, s int
	switch kind {
	case "ClientConnect", "ClientDisconnect", "ClientBegin", "ClientUserinfoChanged":
		if len(fields) < 3 || !slot(fields[2]) {
			return 0, "", invalid
		}
	case "ClientIdentity":
		if len(fields) != 4 || !slot(fields[2]) || !PlayerID(fields[3]) {
			return 0, "", invalid
		}
	case "Kill":
		if n, _ := fmt.Sscanf(raw, "%d:%d Kill: %d %d %d:", &m, &s, &a, &b, &c); n != 5 || a < 0 || a > 1023 || b < 0 || b >= 64 || c < 0 || c > 65535 {
			return 0, "", invalid
		}
	case "score":
		if n, _ := fmt.Sscanf(raw, "%d:%d score: %d ping: %d client: %d", &m, &s, &a, &b, &c); n != 5 || !signedStat(a) || !signedStat(b) || c < 0 || c >= 64 {
			return 0, "", invalid
		}
	}
	return int(minutes*60 + seconds), kind, nil
}

func NewMatchEvent(match string, sequence int64, raw string) (MatchEvent, error) {
	timestamp, kind, err := eventHeader(raw)
	if err != nil || !identifier.MatchString(match) || sequence < 0 || sequence > MaxMatchOffset {
		return MatchEvent{}, errors.New("invalid match event")
	}
	return MatchEvent{1, match, sequence, timestamp, kind, base64.StdEncoding.EncodeToString([]byte(raw))}, nil
}
func (e MatchEvent) Raw() (string, error) {
	if len(e.Payload) > 5460 {
		return "", errors.New("event payload exceeds bound")
	}
	data, err := base64.StdEncoding.DecodeString(e.Payload)
	if err != nil || base64.StdEncoding.EncodeToString(data) != e.Payload {
		return "", errors.New("invalid event encoding")
	}
	return string(data), nil
}
func (e MatchEvent) Validate() error {
	raw, err := e.Raw()
	if err != nil {
		return err
	}
	expected, err := NewMatchEvent(e.MatchID, e.Sequence, raw)
	if err != nil || expected != e {
		return errors.New("event envelope disagrees with payload")
	}
	return nil
}
func (b MatchBatch) Validate() error {
	if b.Version != 1 || !identifier.MatchString(b.MatchID) || b.Sequence < 0 || b.End < b.Sequence || b.End > MaxMatchOffset || b.End-b.Sequence > 12288 || len(b.Events) > 64 || (b.Sequence == b.End && !b.Final) ||
		b.Checkpoint.Version != 1 || b.Checkpoint.MatchID != b.MatchID || b.Checkpoint.Sequence != b.End {
		return errors.New("invalid batch envelope")
	}
	if err := b.Checkpoint.State.Validate(); err != nil {
		return err
	}
	offset := b.Sequence
	for _, event := range b.Events {
		if event.MatchID != b.MatchID || event.Sequence != offset || event.Validate() != nil {
			return errors.New("invalid or noncontiguous event")
		}
		raw, _ := event.Raw()
		offset += int64(len(raw) + 1)
	}
	if offset != b.End {
		return errors.New("event byte offsets disagree")
	}
	return nil
}
func DecodeMatchBatch(data []byte) (MatchBatch, error) {
	var b MatchBatch
	if err := Decode(data, &b); err != nil {
		return b, err
	}
	// Zero offsets/timestamps and false terminal flags are valid, but their
	// absence or JSON null is not part of the versioned wire schema.
	object := func(data []byte, keys ...string) (map[string]json.RawMessage, error) {
		var fields map[string]json.RawMessage
		if err := json.Unmarshal(data, &fields); err != nil {
			return nil, err
		}
		for _, key := range keys {
			if len(fields[key]) == 0 || string(fields[key]) == "null" {
				return nil, errors.New("missing required batch field")
			}
		}
		return fields, nil
	}
	fields, err := object(data, "version", "match_id", "sequence", "end", "events", "checkpoint", "final")
	if err != nil {
		return b, err
	}
	var events []json.RawMessage
	if err = json.Unmarshal(fields["events"], &events); err != nil {
		return b, err
	}
	for _, raw := range events {
		if _, err = object(raw, "version", "match_id", "sequence", "timestamp", "type", "payload"); err != nil {
			return b, err
		}
	}
	cp, err := object(fields["checkpoint"], "version", "match_id", "sequence", "state")
	if err != nil {
		return b, err
	}
	state, err := object(cp["state"], "seconds", "joins", "kills", "completed")
	if err != nil || len(state["players"]) == 0 || len(state["scores"]) == 0 {
		return b, errors.New("missing required checkpoint field")
	}
	for _, name := range []string{"accounts", "owners", "owner_scores", "players", "scores"} {
		raw, present := state[name]
		if !present {
			continue
		}
		if string(raw) == "null" {
			if name == "players" || name == "scores" {
				continue // Legacy empty checkpoint representation is part of v1.
			}
			return b, errors.New("null checkpoint map")
		}
		values, err := object(raw)
		if err != nil {
			return b, err
		}
		for _, value := range values {
			if string(value) == "null" {
				return b, errors.New("null checkpoint value")
			}
			if name == "accounts" {
				_, err = object(value, "kills", "deaths", "score")
			} else if name == "players" {
				_, err = object(value, "kills", "deaths")
			}
			if err != nil {
				return b, err
			}
		}
	}
	return b, b.Validate()
}
