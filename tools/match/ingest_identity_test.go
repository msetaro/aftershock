package main

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"testing"

	"github.com/msetaro/aftershock/tools/match/contracts"
)

func TestIngestAgonesIdentity(t *testing.T) {
	allocation := struct {
		Match   contracts.MatchSpec `json:"match"`
		JoinKey string              `json:"join_key"`
		Token   string              `json:"token"`
	}{contracts.MatchSpec{Version: 1, ID: "identity-match", Map: "two_lane", Mode: 0, Slots: 2,
		Rules: contracts.Rules{TimeLimit: 1}, ExpectedPlayers: []string{"18446744073709551615"}}, strings.Repeat("12", 32), "private-allocation-token"}
	encoded, _ := json.Marshal(allocation)
	item := map[string]any{
		"metadata": map[string]any{"name": "server-1", "labels": map[string]string{"aftershock.dev/match": "identity-match"},
			"annotations": map[string]string{"aftershock.dev/match": string(encoded)}},
		"status": map[string]string{"state": "Allocated"},
	}
	items := []any{item}
	var mu sync.Mutex
	tokenFile := filepath.Join(t.TempDir(), "token")
	if err := os.WriteFile(tokenFile, []byte("private-kube-token"), 0600); err != nil {
		t.Fatal(err)
	}
	server := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		mu.Lock()
		defer mu.Unlock()
		if r.URL.Path != "/apis/agones.dev/v1/namespaces/ingest-test/gameservers" ||
			r.URL.Query().Get("labelSelector") != "aftershock.dev/match=identity-match" ||
			r.Header.Get("Authorization") != "Bearer private-kube-token" {
			t.Error("incorrect bounded namespace/identity lookup")
			w.WriteHeader(http.StatusForbidden)
			return
		}
		_ = json.NewEncoder(w).Encode(map[string]any{"items": items})
	}))
	defer server.Close()
	cluster := kubernetesClient{url: server.URL, tokenFile: tokenFile, namespace: "ingest-test", client: server.Client()}
	verify := func(token string) ([]string, error) {
		return cluster.verifyAllocation(context.Background(), "identity-match", token)
	}
	players, err := verify("private-allocation-token")
	if err != nil || len(players) != 1 || players[0] != "18446744073709551615" {
		t.Fatal("allocated identity rejected", err)
	}
	if _, err := verify("wrong-token"); err == nil {
		t.Fatal("wrong allocation token accepted")
	}
	mu.Lock()
	item["status"] = map[string]string{"state": "Ready"}
	mu.Unlock()
	if _, err := verify("private-allocation-token"); err == nil {
		t.Fatal("unallocated GameServer accepted")
	}
	mu.Lock()
	item["status"] = map[string]string{"state": "Allocated"}
	items = []any{item, item}
	mu.Unlock()
	if _, err := verify("private-allocation-token"); err == nil {
		t.Fatal("ambiguous allocation accepted")
	}
	mu.Lock()
	items = nil
	mu.Unlock()
	if _, err := verify("private-allocation-token"); err == nil {
		t.Fatal("missing allocation accepted")
	}
	mu.Lock()
	items = []any{item}
	allocation.Match.ID = "different-match"
	encoded, _ = json.Marshal(allocation)
	item["metadata"].(map[string]any)["annotations"] = map[string]string{"aftershock.dev/match": string(encoded)}
	mu.Unlock()
	if _, err := verify("private-allocation-token"); err == nil {
		t.Fatal("annotation match mismatch accepted")
	}
}
