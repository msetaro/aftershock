package contracts

import (
	"encoding/base64"
	"encoding/json"
	"strings"
	"testing"
)

// Draft test contract for #30. Commit on the issue branch before implementation.
func TestMatchEventWirePreservesRawBytes(t *testing.T) {
	raw := "  0:12 ClientUserinfoChanged: 0 n\\" + string([]byte{0xff, 0x80})
	e, err := NewMatchEvent("match-1", 0, raw)
	if err != nil {
		t.Fatal(err)
	}
	if e.Version != 1 || e.MatchID != "match-1" || e.Sequence != 0 || e.Timestamp != 12 || e.Type != "ClientUserinfoChanged" {
		t.Fatalf("wrong event envelope: %+v", e)
	}
	data, err := json.Marshal(e)
	if err != nil {
		t.Fatal(err)
	}
	var decoded MatchEvent
	if err = json.Unmarshal(data, &decoded); err != nil {
		t.Fatal(err)
	}
	bytes, err := base64.StdEncoding.DecodeString(decoded.Payload)
	if err != nil || string(bytes) != raw {
		t.Fatal("event transport changed original log bytes")
	}
	if err = decoded.Validate(); err != nil {
		t.Fatal(err)
	}
}

func TestMatchBatchContract(t *testing.T) {
	raw := "  0:00 InitGame: \\mapname\\two_lane"
	e, err := NewMatchEvent("match-1", 0, raw)
	if err != nil {
		t.Fatal(err)
	}
	valid := MatchBatch{Version: 1, MatchID: "match-1", Sequence: 0,
		End: int64(len(raw) + 1), Events: []MatchEvent{e},
		Checkpoint: MatchCheckpoint{Version: 1, MatchID: "match-1", Sequence: int64(len(raw) + 1)}}
	if err := valid.Validate(); err != nil {
		t.Fatal(err)
	}
	clone := func() MatchBatch {
		data, _ := json.Marshal(valid)
		var result MatchBatch
		if err := json.Unmarshal(data, &result); err != nil {
			t.Fatal(err)
		}
		return result
	}
	cases := map[string]func(*MatchBatch){
		"version":              func(b *MatchBatch) { b.Version = 2 },
		"match":                func(b *MatchBatch) { b.MatchID = "../match" },
		"negative sequence":    func(b *MatchBatch) { b.Sequence = -1 },
		"large sequence":       func(b *MatchBatch) { b.End = 1<<33 + 1 },
		"missing bytes":        func(b *MatchBatch) { b.End++ },
		"wrong event match":    func(b *MatchBatch) { b.Events[0].MatchID = "other" },
		"wrong event sequence": func(b *MatchBatch) { b.Events[0].Sequence++ },
		"wrong event time":     func(b *MatchBatch) { b.Events[0].Timestamp++ },
		"wrong event type":     func(b *MatchBatch) { b.Events[0].Type = "Kill" },
		"invalid base64":       func(b *MatchBatch) { b.Events[0].Payload = "!" },
		"event version":        func(b *MatchBatch) { b.Events[0].Version = 2 },
		"checkpoint version":   func(b *MatchBatch) { b.Checkpoint.Version = 2 },
		"checkpoint match":     func(b *MatchBatch) { b.Checkpoint.MatchID = "other" },
		"checkpoint sequence":  func(b *MatchBatch) { b.Checkpoint.Sequence++ },
	}
	for name, mutate := range cases {
		t.Run(name, func(t *testing.T) {
			b := clone()
			mutate(&b)
			if b.Validate() == nil {
				t.Fatal("invalid batch accepted")
			}
		})
	}
	data, _ := json.Marshal(valid)
	if _, err := DecodeMatchBatch(data); err != nil {
		t.Fatal(err)
	}
	for _, suffix := range []string{`,"unknown":1}`, `,"version":2}`} {
		invalid := append(append([]byte(nil), data[:len(data)-1]...), suffix...)
		if _, err := DecodeMatchBatch(invalid); err == nil {
			t.Fatal("unknown/unsupported batch field accepted")
		}
	}
	if _, err := DecodeMatchBatch(append(data, []byte(" {}")...)); err == nil {
		t.Fatal("trailing JSON accepted")
	}
	// A terminal empty batch is necessary when the engine exits before writing
	// another log line; an empty non-terminal batch cannot advance a stream.
	empty := clone()
	empty.Sequence = empty.End
	empty.Events = nil
	if empty.Validate() == nil {
		t.Fatal("non-terminal empty batch accepted")
	}
	empty.Final = true
	if err := empty.Validate(); err != nil {
		t.Fatal(err)
	}
}

func TestMatchEventBounds(t *testing.T) {
	for _, raw := range []string{"  0:00 Exit: done\n", "  0:00 Exit: done\r", strings.Repeat("x", 4096)} {
		if _, err := NewMatchEvent("match-1", 0, raw); err == nil {
			t.Fatal("unbounded or multi-line event accepted")
		}
	}
	for _, raw := range []string{
		"  0:00 Kill: malformed", "  0:00 ClientIdentity: 0 0",
		"  0:00 ClientIdentity: 64 1", "  0:00 ClientIdentity: 0 01",
		"  0:00 ClientConnect: -1", "  0:00 ClientBegin: 64",
		"  0:00 score: missing", "  0:60 Exit: invalid time",
	} {
		if _, err := NewMatchEvent("match-1", 0, raw); err == nil {
			t.Fatal("malformed recognized event accepted", raw)
		}
	}
	// Unknown bounded legacy records remain explicit data, never silently lost.
	for _, raw := range []string{"  0:00 ------------------------------------------------------------", "unknown legacy record"} {
		e, err := NewMatchEvent("match-1", 1, raw)
		if err != nil || e.Validate() != nil {
			t.Fatal("bounded legacy record rejected", err)
		}
	}
}
