package main

import (
	"context"
	"os"
	"path/filepath"
	"time"
)

// Exec preStop for both match containers. A shared marker requests engine quit;
// it never starts a second cursor writer or invents a successful ingest ACK.
func drain(ctx context.Context) error {
	home := env("MATCH_HOME", "/home/match")
	if !exists(filepath.Join(home, "match.json")) || exists(filepath.Join(home, "results.done")) {
		return nil
	}
	marker, err := os.OpenFile(filepath.Join(home, "drain.request"), os.O_WRONLY|os.O_CREATE, 0600)
	if err != nil {
		return err
	}
	err = marker.Sync()
	closed := marker.Close()
	if err != nil {
		return err
	}
	if closed != nil {
		return closed
	}
	bounded, cancel := context.WithTimeout(ctx, 65*time.Second)
	defer cancel()
	for !exists(filepath.Join(home, "results.done")) {
		if err := pause(bounded, 100*time.Millisecond); err != nil {
			return err
		}
	}
	return nil
}
