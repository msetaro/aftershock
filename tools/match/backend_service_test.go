//go:build backend_integration

package main

import (
	"bytes"
	"context"
	"crypto/x509"
	"encoding/json"
	"encoding/pem"
	"fmt"
	"io"
	"net"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"sync/atomic"
	"testing"
	"time"
)

// The driver supplies a private, disposable real PostgreSQL instance. No mock SQL.
func TestBackendAuthProfile(t *testing.T) {
	dsn := os.Getenv("BACKEND_TEST_DATABASE")
	if dsn == "" {
		t.Fatal("BACKEND_TEST_DATABASE is required; run tests/backend_services.py")
	}
	var redirected atomic.Int32
	steam := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/redirected" {
			redirected.Add(1)
			http.Error(w, "unexpected redirect", 500)
			return
		}
		if r.Method != "GET" || r.ParseForm() != nil || r.Form.Get("key") != "private-test-key" || r.Form.Get("appid") != "12345" || r.Form.Get("identity") != "aftershock" {
			http.Error(w, "invalid test exchange", 400)
			return
		}
		id := "18446744073709551615"
		switch r.Form.Get("ticket") {
		case "aa00", "aa01":
		case "bb00":
			id = "123"
		case "cc00":
			id = "18446744073709551616"
		case "ee00":
			http.Redirect(w, r, "https://"+r.Host+"/redirected", 307)
			return
		default:
			http.Error(w, "rejected", 403)
			return
		}
		fmt.Fprintf(w, `{"response":{"params":{"result":"OK","steamid":"%s","ownersteamid":"%s"}}}`, id, id)
	}))
	defer steam.Close()
	db, err := openBackendDB(context.Background(), dsn)
	if err != nil {
		t.Fatal(err)
	}
	defer func() { db.Close() }()
	service := &backendService{db: db, steamURL: steam.URL, steamKey: "private-test-key", appID: "12345", client: steam.Client(),
		catalog: map[string]bool{"weapons/range_rifle.asweapon": true, "weapons/second.asweapon": true}}
	call := func(method, path, token, body string, want int) map[string]any {
		t.Helper()
		r := httptest.NewRequest(method, "https://backend.test"+path, bytes.NewBufferString(body))
		if token != "" {
			r.Header.Set("Authorization", "Bearer "+token)
		}
		w := httptest.NewRecorder()
		service.ServeHTTP(w, r)
		if w.Code != want {
			t.Fatalf("%s %s: got %d want %d: %s", method, path, w.Code, want, w.Body.String())
		}
		var reply map[string]any
		if err := json.Unmarshal(w.Body.Bytes(), &reply); err != nil {
			t.Fatal(err)
		}
		return reply
	}
	call("GET", "/v1/profile", "", "", 401)
	call("POST", "/v1/login", "", `{"ticket":"zz"}`, 400)
	call("POST", "/v1/login", "", `{"ticket":"dd00"}`, 401)
	call("POST", "/v1/login", "", `{"ticket":"cc00"}`, 401)
	call("POST", "/v1/login", "", `{"ticket":"ee00"}`, 503)
	if redirected.Load() != 0 {
		t.Fatal("authentication followed a redirect")
	}
	call("POST", "/v1/login", "", `{"ticket":"`+strings.Repeat("aa", 5000)+`"}`, 400)
	call("POST", "/v1/login", "", `{"ticket":"aa00","player_id":"123"}`, 400)
	login := call("POST", "/v1/login", "", `{"ticket":"aa00"}`, 200)
	if login["player_id"] != "18446744073709551615" || login["version"] != float64(1) {
		t.Fatal(login)
	}
	token, ok := login["token"].(string)
	if !ok || len(token) != 64 {
		t.Fatal("missing bounded opaque session")
	}
	call("POST", "/v1/login", "", `{"ticket":"aa00"}`, 401)
	call("GET", "/v1/profile", "bad-token", "", 401)
	profile := call("GET", "/v1/profile", token, "", 200)
	if profile["player_id"] != login["player_id"] {
		t.Fatal(profile)
	}
	good := `{"display_name":"Player ☃","loadout":{"version":1,"primary":"weapons/range_rifle.asweapon","secondary":"weapons/second.asweapon"}}`
	call("PUT", "/v1/profile", token, good, 200)
	call("PUT", "/v1/profile", token, `{"display_name":"x","loadout":{"version":1,"primary":"weapons/missing.asweapon","secondary":"weapons/second.asweapon"}}`, 400)
	call("PUT", "/v1/profile", token, `{"display_name":"x","loadout":{"version":1,"primary":"weapons/range_rifle.asweapon","secondary":"weapons/second.asweapon","damage":999}}`, 400)
	// Recreate the whole pool/service: sessions and profiles must not live in process memory.
	db.Close()
	db, err = openBackendDB(context.Background(), dsn)
	if err != nil {
		t.Fatal(err)
	}
	service.db = db
	profile = call("GET", "/v1/profile", token, "", 200)
	if profile["display_name"] != "Player ☃" || profile["loadout"].(map[string]any)["secondary"] != "weapons/second.asweapon" {
		t.Fatal(profile)
	}
	// Concurrent replica attempts cannot exchange one provider ticket twice.
	results := make(chan int, 8)
	for range 8 {
		go func() {
			r := httptest.NewRequest("POST", "https://backend.test/v1/login", strings.NewReader(`{"ticket":"aa01"}`))
			w := httptest.NewRecorder()
			service.ServeHTTP(w, r)
			results <- w.Code
		}()
	}
	accepted := 0
	for range 8 {
		status := <-results
		if status == 200 {
			accepted++
		} else if status != 401 {
			t.Fatal("concurrent exchange", status)
		}
	}
	if accepted != 1 {
		t.Fatal("ticket replay across replicas", accepted)
	}
	other := call("POST", "/v1/login", "", `{"ticket":"bb00"}`, 200)["token"].(string)
	if call("GET", "/v1/profile", other, "", 200)["display_name"] == "Player ☃" {
		t.Fatal("profile ownership crossed accounts")
	}
	call("POST", "/v1/logout", token, `{}`, 200)
	call("GET", "/v1/profile", token, "", 401)
	call("GET", "/v1/profile", other, "", 200)
	if _, err := db.Exec("UPDATE backend_sessions SET expires_at = CURRENT_TIMESTAMP - INTERVAL '1 second'"); err != nil {
		t.Fatal(err)
	}
	call("GET", "/v1/profile", other, "", 401)
}

func TestBackendHTTPS(t *testing.T) {
	steam := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Query().Get("ticket") != "dd01" || r.URL.Query().Get("identity") != "aftershock" {
			http.Error(w, "rejected", 403)
			return
		}
		fmt.Fprint(w, `{"response":{"params":{"result":"OK","steamid":"789"}}}`)
	}))
	defer steam.Close()
	root := t.TempDir()
	cert := filepath.Join(root, "cert.pem")
	key := filepath.Join(root, "key.pem")
	private, err := x509.MarshalPKCS8PrivateKey(steam.TLS.Certificates[0].PrivateKey)
	if err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(cert, pem.EncodeToMemory(&pem.Block{Type: "CERTIFICATE", Bytes: steam.Certificate().Raw}), 0600); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(key, pem.EncodeToMemory(&pem.Block{Type: "PRIVATE KEY", Bytes: private}), 0600); err != nil {
		t.Fatal(err)
	}
	if err := os.WriteFile(filepath.Join(root, "steam-key"), []byte("private-test-key"), 0600); err != nil {
		t.Fatal(err)
	}
	if err := writeJSON(filepath.Join(root, "catalog.json"), []string{"weapons/range_rifle.asweapon"}); err != nil {
		t.Fatal(err)
	}
	reservation, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}
	address := reservation.Addr().String()
	reservation.Close()
	for name, value := range map[string]string{
		"BACKEND_DATABASE": os.Getenv("BACKEND_TEST_DATABASE"), "BACKEND_LISTEN": address,
		"BACKEND_TLS_CERT": cert, "BACKEND_TLS_KEY": key, "BACKEND_CA_FILE": cert,
		"BACKEND_STEAM_URL": steam.URL, "BACKEND_STEAM_KEY_FILE": filepath.Join(root, "steam-key"),
		"BACKEND_APP_ID": "12345", "BACKEND_CATALOG": filepath.Join(root, "catalog.json"),
	} {
		t.Setenv(name, value)
	}
	ctx, cancel := context.WithCancel(context.Background())
	done := make(chan error, 1)
	go func() { done <- serveBackend(ctx) }()
	defer func() {
		cancel()
		if err := <-done; err != nil {
			t.Error(err)
		}
	}()
	client := steam.Client()
	client.Timeout = 3 * time.Second
	deadline := time.Now().Add(10 * time.Second)
	for {
		response, err := client.Get("https://" + address + "/healthz")
		if err == nil {
			response.Body.Close()
			if response.StatusCode == 200 {
				break
			}
		}
		if time.Now().After(deadline) {
			t.Fatal("HTTPS service never became ready")
		}
		time.Sleep(20 * time.Millisecond)
	}
	response, err := client.Post("https://"+address+"/v1/login", "application/json", strings.NewReader(`{"ticket":"dd01"}`))
	if err != nil {
		t.Fatal(err)
	}
	defer response.Body.Close()
	var login map[string]any
	if err := json.NewDecoder(response.Body).Decode(&login); err != nil || response.StatusCode != 200 || login["player_id"] != "789" {
		t.Fatal("HTTPS login failed", err, response.StatusCode)
	}
	metrics, err := client.Get("https://" + address + "/metrics")
	if err != nil {
		t.Fatal(err)
	}
	defer metrics.Body.Close()
	data, err := io.ReadAll(metrics.Body)
	if err != nil || !bytes.Contains(data, []byte(`aftershock_backend_requests_total{service="auth"} 1`)) || bytes.Contains(data, []byte(login["token"].(string))) {
		t.Fatal("missing bounded service metrics", err)
	}
}
