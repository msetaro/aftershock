package main

import (
	"context"
	"net"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"
)

const goodSpec = `{"id":"local-1","map":"two_lane","mode":0,"frag_limit":1,"time_limit":1,"players":2,"password":"local-secret","token":"allocation-secret"}`

func TestSpec(t *testing.T) {
	s, err := decodeSpec([]byte(goodSpec))
	if err != nil {
		t.Fatal(err)
	}
	if s.Map != "two_lane" || s.Players != 2 {
		t.Fatal(s)
	}
	for _, bad := range []string{
		strings.Replace(goodSpec, `"two_lane"`, `"two_lane;quit"`, 1),
		strings.Replace(goodSpec, `"two_lane"`, `"`+strings.Repeat("a", 64)+`"`, 1),
		strings.Replace(goodSpec, `"players":2`, `"players":0`, 1),
		strings.Replace(goodSpec, `"mode":0`, `"mode":99`, 1),
		strings.Replace(goodSpec, `"password":"local-secret"`, `"password":""`, 1),
		strings.Replace(strings.Replace(goodSpec, `"frag_limit":1`, `"frag_limit":0`, 1), `"time_limit":1`, `"time_limit":0`, 1),
		strings.Replace(goodSpec, `"token":"allocation-secret"`, `"extra":true,"token":"allocation-secret"`, 1),
		goodSpec + `{}`, strings.Repeat(" ", 65537) + goodSpec,
	} {
		if _, err := decodeSpec([]byte(bad)); err == nil {
			t.Fatalf("accepted invalid spec: %.200s", bad)
		}
	}
}

func TestOwnedArguments(t *testing.T) {
	s, _ := decodeSpec([]byte(goodSpec))
	a, err := serverArgs(s, "/content", "/home/match", "aftershock", 27960, false)
	if err != nil {
		t.Fatal(err)
	}
	text := strings.Join(a, " ")
	for _, want := range []string{"+set fs_basegame aftershock", "+map two_lane", "+set sv_exitOnMatchEnd 1", "+set g_password local-secret"} {
		if !strings.Contains(text, want) {
			t.Fatal(text)
		}
	}
	if _, err := serverArgs(s, "/content", "/home/match", "../baseq3", 27960, false); err == nil {
		t.Fatal("accepted invalid content name")
	}
}

func TestCheckpoint(t *testing.T) {
	c := checkpoint{}
	for _, line := range []string{"  0:00 ClientConnect: 1", "  0:01 ClientBegin: 1", "  0:02 Kill: 1 2 7: Player killed Other by MOD_ROCKET", "  1:00 score: 3  ping: 5  client: 1 Player", "  1:00 Exit: Fraglimit hit.", "  1:05 ShutdownGame:"} {
		c.add(line)
	}
	if c.Kills != 1 || c.Joins != 1 || c.Scores["1"] != 3 || !c.Completed {
		t.Fatal(c)
	}
}

func TestIngestDurability(t *testing.T) {
	path := filepath.Join(t.TempDir(), "events.jsonl")
	s, err := newIngest(path, map[string]string{"local-1": "allocation-secret"})
	if err != nil {
		t.Fatal(err)
	}
	b := batch{Version: 1, Match: "local-1", Start: 0, End: int64(len("  0:00 InitGame: test\n")), Events: []string{"  0:00 InitGame: test"}}
	if err = s.accept(b, "wrong"); err == nil {
		t.Fatal("wrong token accepted")
	}
	if err = s.accept(b, "allocation-secret"); err != nil {
		t.Fatal(err)
	}
	if err = s.accept(b, "allocation-secret"); err != nil {
		t.Fatal(err)
	}
	s.file.Close()
	s, err = newIngest(path, map[string]string{"local-1": "allocation-secret"})
	if err != nil {
		t.Fatal(err)
	}
	defer s.file.Close()
	if err = s.accept(b, "allocation-secret"); err != nil {
		t.Fatal(err)
	}
	b.Start = 99
	b.End = 100
	if err = s.accept(b, "allocation-secret"); err == nil {
		t.Fatal("accepted a gap")
	}
	data, _ := os.ReadFile(path)
	if strings.Count(string(data), "\n") != 1 {
		t.Fatalf("duplicate durable write: %s", data)
	}
}

func TestShipperRetry(t *testing.T) {
	home := t.TempDir()
	s, _ := decodeSpec([]byte(goodSpec))
	if err := writeJSON(filepath.Join(home, "match.json"), s); err != nil {
		t.Fatal(err)
	}
	os.Mkdir(filepath.Join(home, "aftershock"), 0700)
	logPath := filepath.Join(home, "aftershock", "games.log")
	if err := os.WriteFile(logPath, []byte("  0:00 ClientBegin: 1\n"), 0600); err != nil {
		t.Fatal(err)
	}
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}
	address := listener.Addr().String()
	listener.Close()
	ctx, cancel := context.WithTimeout(context.Background(), 15*time.Second)
	defer cancel()
	result := make(chan error, 1)
	go func() { result <- ship(ctx, home, "aftershock", address, true) }()
	deadline := time.Now().Add(5 * time.Second)
	for !exists(filepath.Join(home, "pending.json")) && time.Now().Before(deadline) {
		time.Sleep(10 * time.Millisecond)
	}
	if !exists(filepath.Join(home, "pending.json")) || exists(filepath.Join(home, "ack.json")) {
		t.Fatal("offline batch was not retained unacknowledged")
	}
	store := filepath.Join(t.TempDir(), "events.jsonl")
	serverResult := make(chan error, 1)
	go func() { serverResult <- serveIngest(ctx, address, store, map[string]string{s.ID: s.Token}) }()
	f, err := os.OpenFile(logPath, os.O_APPEND|os.O_WRONLY, 0600)
	if err != nil {
		t.Fatal(err)
	}
	f.WriteString("  1:00 Exit: Timelimit hit.\n  1:05 ShutdownGame:\n")
	f.Close()
	if err = writeJSON(filepath.Join(home, "engine.done"), map[string]bool{"clean": true}); err != nil {
		t.Fatal(err)
	}
	if err = <-result; err != nil {
		t.Fatal(err)
	}
	var ack cursor
	if err = readJSON(filepath.Join(home, "ack.json"), &ack); err != nil {
		t.Fatal(err)
	}
	info, _ := os.Stat(logPath)
	if ack.End != info.Size() || !ack.Final || !ack.Checkpoint.Completed || ack.Checkpoint.Joins != 1 || !exists(filepath.Join(home, "results.done")) {
		t.Fatalf("bad final cursor: %+v", ack)
	}
	cancel()
	if err = <-serverResult; err != nil {
		t.Fatal(err)
	}
}

func TestIncompleteDurableRecord(t *testing.T) {
	path := filepath.Join(t.TempDir(), "events.jsonl")
	if err := os.WriteFile(path, []byte(`{"version":1}`), 0600); err != nil {
		t.Fatal(err)
	}
	if s, err := newIngest(path, map[string]string{}); err == nil {
		s.file.Close()
		t.Fatal("accepted an incomplete durable record")
	}
}

func TestExitBeforeGameLog(t *testing.T) {
	home := t.TempDir()
	s, _ := decodeSpec([]byte(goodSpec))
	if err := writeJSON(filepath.Join(home, "match.json"), s); err != nil {
		t.Fatal(err)
	}
	if err := writeJSON(filepath.Join(home, "engine.done"), map[string]bool{"clean": false}); err != nil {
		t.Fatal(err)
	}
	listener, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}
	address := listener.Addr().String()
	listener.Close()
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	serverResult := make(chan error, 1)
	go func() {
		serverResult <- serveIngest(ctx, address, filepath.Join(t.TempDir(), "events.jsonl"), map[string]string{s.ID: s.Token})
	}()
	if err = ship(ctx, home, "aftershock", address, true); err != nil {
		t.Fatal(err)
	}
	var ack cursor
	if err = readJSON(filepath.Join(home, "ack.json"), &ack); err != nil {
		t.Fatal(err)
	}
	if !ack.Final || ack.End != 0 || ack.Checkpoint.Completed {
		t.Fatalf("startup failure falsely reported completion: %+v", ack)
	}
	cancel()
	if err = <-serverResult; err != nil {
		t.Fatal(err)
	}
}

func TestFinalStream(t *testing.T) {
	path := filepath.Join(t.TempDir(), "events.jsonl")
	tokens := map[string]string{"local-1": "allocation-secret"}
	s, err := newIngest(path, tokens)
	if err != nil {
		t.Fatal(err)
	}
	final := batch{Version: 1, Match: "local-1", Final: true}
	if err = s.accept(final, "allocation-secret"); err != nil {
		t.Fatal(err)
	}
	s.file.Close()
	s, err = newIngest(path, tokens)
	if err != nil {
		t.Fatal(err)
	}
	defer s.file.Close()
	if err = s.accept(final, "allocation-secret"); err != nil {
		t.Fatal(err)
	}
	if err = s.accept(batch{Version: 1, Match: "local-1", End: 6, Events: []string{"event"}}, "allocation-secret"); err == nil {
		t.Fatal("accepted data after final acknowledgement")
	}
}
