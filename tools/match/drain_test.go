package main

import (
	"context"
	"errors"
	"os"
	"path/filepath"
	"testing"
	"time"
)

func TestDrainWaitsForAcknowledgement(t *testing.T) {
	home := t.TempDir()
	t.Setenv("MATCH_HOME", home)
	if err := drain(context.Background()); err != nil {
		t.Fatal("warm unallocated pod should have nothing to drain", err)
	}
	if err := os.WriteFile(filepath.Join(home, "match.json"), []byte("{}"), 0600); err != nil {
		t.Fatal(err)
	}
	ctx, cancel := context.WithTimeout(context.Background(), 3*time.Second)
	defer cancel()
	done := make(chan error, 1)
	go func() { done <- drain(ctx) }()
	for !exists(filepath.Join(home, "drain.request")) {
		if ctx.Err() != nil {
			t.Fatal("drain did not request graceful engine completion")
		}
		time.Sleep(time.Millisecond)
	}
	select {
	case err := <-done:
		t.Fatal("preStop returned before durable acknowledgement", err)
	case <-time.After(20 * time.Millisecond):
	}
	if err := os.WriteFile(filepath.Join(home, "results.done"), []byte("{}"), 0600); err != nil {
		t.Fatal(err)
	}
	if err := <-done; err != nil {
		t.Fatal(err)
	}
	if err := os.Remove(filepath.Join(home, "results.done")); err != nil {
		t.Fatal(err)
	}
	canceled, stop := context.WithCancel(context.Background())
	stop()
	if err := drain(canceled); !errors.Is(err, context.Canceled) {
		t.Fatal("termination deadline must remain bounded", err)
	}
	if exists(filepath.Join(home, "results.done")) {
		t.Fatal("drain manufactured an acknowledgement")
	}
}
