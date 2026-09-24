package main

import (
	"encoding/base64"
	"encoding/json"
	"path/filepath"
	"testing"
)

func TestIngestPendingPreservesLogBytes(t *testing.T) {
	raw := "  0:01 ClientUserinfoChanged: 0 n\\" + string([]byte{0xff, 0x80})
	b := batch{Version: 1, Match: "raw-match", Events: []string{raw}, End: int64(len(raw) + 1)}
	b.Checkpoint.add(raw)
	path := filepath.Join(t.TempDir(), "pending.json")
	if err := writeJSON(path, b); err != nil {
		t.Fatal(err)
	}
	var recovered batch
	if err := readJSON(path, &recovered); err != nil {
		t.Fatal(err)
	}
	if len(recovered.Events) != 1 || recovered.Events[0] != raw || recovered.End != b.End {
		t.Fatal("pending cursor recovery changed raw log bytes")
	}
	wire, err := matchBatch(recovered)
	if err != nil {
		t.Fatal(err)
	}
	decoded, err := base64.StdEncoding.DecodeString(wire.Events[0].Payload)
	if err != nil || string(decoded) != raw || wire.End != int64(len(decoded)+1) {
		t.Fatal("wire no longer agrees with native log byte offsets")
	}
	encoded, err := json.Marshal(wire)
	if err != nil {
		t.Fatal(err)
	}
	var fields map[string]any
	if err := json.Unmarshal(encoded, &fields); err != nil {
		t.Fatal(err)
	}
	if fields["sequence"] != float64(0) || fields["end"] != float64(b.End) {
		t.Fatal("wire sequence contract changed")
	}
}

func TestIngestLegacyPendingCompatibility(t *testing.T) {
	// This is the existing #28 disk contract, including a zero checkpoint. New
	// disk support must read it and keep ordinary ASCII batches in this shape.
	legacy := []byte(`{"version":1,"match":"legacy-1","start":0,"end":0,"events":null,"checkpoint":{"seconds":0,"players":null,"joins":0,"kills":0,"scores":null,"completed":false},"final":true}`)
	var b batch
	if err := strictJSON(legacy, &b); err != nil {
		t.Fatal(err)
	}
	data, err := json.Marshal(b)
	if err != nil || string(data) != string(legacy) {
		t.Fatal("legacy ASCII pending representation changed")
	}
	wire, err := matchBatch(b)
	if err != nil || !wire.Final || len(wire.Events) != 0 {
		t.Fatal("legacy terminal empty batch cannot resume", err)
	}
}
