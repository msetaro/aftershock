package main

import (
	"context"
	"errors"
	"fmt"
	"os"
	"os/signal"
	"path/filepath"
	"syscall"
)

func main() {
	ctx, cancel := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer cancel()
	var err error
	if len(os.Args) != 2 {
		err = errors.New("usage: match run|ship|stub|probe")
	} else {
		switch os.Args[1] {
		case "run":
			err = runServer(ctx)
		case "ship":
			err = ship(ctx, env("MATCH_HOME", "/home/match"), env("MATCH_GAME", "aftershock"), env("MATCH_INGEST", "ingest:50051"), os.Getenv("MATCH_DEV_INSECURE") == "1")
		case "probe":
			err = probe("127.0.0.1:"+env("MATCH_PORT", "27960"), env("MATCH_MAP", "two_lane"))
		case "stub":
			if os.Getenv("MATCH_DEV_INSECURE") != "1" {
				err = errors.New("stub is local-development only; requires MATCH_DEV_INSECURE=1")
				break
			}
			var tokens map[string]string
			err = strictJSON([]byte(os.Getenv("MATCH_TOKENS")), &tokens)
			if err == nil {
				err = serveIngest(ctx, env("MATCH_LISTEN", ":50051"), filepath.Join(env("MATCH_STATE", "/state"), "events.jsonl"), tokens)
			}
		default:
			err = errors.New("unknown match command")
		}
	}
	if err != nil {
		fmt.Fprintln(os.Stderr, "match:", err)
		os.Exit(1)
	}
}
