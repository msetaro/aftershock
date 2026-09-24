package main

import (
	"context"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"sync/atomic"
	"testing"
)

func TestBackendProjectedCredentialRotation(t *testing.T) {
	path := filepath.Join(t.TempDir(), "token")
	write := func(value string) {
		t.Helper()
		if err := os.WriteFile(path+".next", []byte(value), 0600); err != nil {
			t.Fatal(err)
		}
		if err := os.Rename(path+".next", path); err != nil {
			t.Fatal(err)
		}
	}
	var calls atomic.Int32
	server := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		n := calls.Add(1)
		want := "Bearer first"
		if n == 2 {
			want = "Bearer rotated"
		}
		if r.Header.Get("Authorization") != want {
			w.WriteHeader(http.StatusUnauthorized)
			return
		}
		_, _ = w.Write([]byte(`{}`))
	}))
	defer server.Close()
	b := backendService{kubeURL: server.URL, kubeTokenFile: path, client: server.Client()}
	request := func() error {
		var out map[string]any
		return b.kube(context.Background(), http.MethodGet, "/", nil, &out)
	}
	for _, value := range []string{"first\n", "rotated\n"} {
		write(value)
		if err := request(); err != nil {
			t.Fatalf("valid projected credential: %v", err)
		}
	}
	for _, value := range []string{"", " \n", strings.Repeat("x", 16385)} {
		write(value)
		if err := request(); err == nil {
			t.Fatal("invalid credential accepted")
		}
	}
	if err := os.Remove(path); err != nil {
		t.Fatal(err)
	}
	if err := request(); err == nil {
		t.Fatal("missing credential accepted")
	}
	if calls.Load() != 2 {
		t.Fatal("invalid/missing credential sent a request or reused a stale token")
	}
}
