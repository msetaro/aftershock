//go:build backend_integration

package main

import (
	"bytes"
	"context"
	"crypto/x509"
	"encoding/hex"
	"encoding/json"
	"encoding/pem"
	"fmt"
	"github.com/msetaro/aftershock/tools/match/contracts"
	"io"
	"net"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"sync"
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

func TestBackendParties(t *testing.T) {
	db, err := openBackendDB(context.Background(), os.Getenv("BACKEND_TEST_DATABASE"))
	if err != nil {
		t.Fatal(err)
	}
	defer db.Close()
	steam := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		ids := map[string]string{"1100": "1001", "1200": "1002", "1300": "1003", "1400": "1004"}
		id := ids[r.URL.Query().Get("ticket")]
		fmt.Fprintf(w, `{"response":{"params":{"result":"OK","steamid":"%s"}}}`, id)
	}))
	defer steam.Close()
	service := &backendService{db: db, steamURL: steam.URL, steamKey: "private-test-key", appID: "12345", client: steam.Client()}
	call := func(method, token, body string, want int) map[string]any {
		t.Helper()
		r := httptest.NewRequest(method, "https://backend.test/v1/party", strings.NewReader(body))
		r.Header.Set("Authorization", "Bearer "+token)
		w := httptest.NewRecorder()
		service.ServeHTTP(w, r)
		if w.Code != want {
			t.Fatalf("party %s: got %d want %d: %s", method, w.Code, want, w.Body.String())
		}
		var result map[string]any
		if err := json.Unmarshal(w.Body.Bytes(), &result); err != nil {
			t.Fatal(err)
		}
		return result
	}
	tokens := make([]string, 4)
	for i, ticket := range []string{"1100", "1200", "1300", "1400"} {
		r := httptest.NewRequest("POST", "https://backend.test/v1/login", strings.NewReader(`{"ticket":"`+ticket+`"}`))
		w := httptest.NewRecorder()
		service.ServeHTTP(w, r)
		var result map[string]any
		if json.Unmarshal(w.Body.Bytes(), &result) != nil || w.Code != 200 {
			t.Fatal("party login failed", w.Code)
		}
		tokens[i] = result["token"].(string)
	}
	if call("GET", tokens[0], "", 200)["party"] != nil {
		t.Fatal("unexpected initial membership")
	}
	first := call("POST", tokens[0], `{}`, 200)["party"].(map[string]any)
	code := first["invite_code"].(string)
	if len(code) != 32 || first["leader"] != "1001" || len(first["members"].([]any)) != 1 {
		t.Fatal(first)
	}
	call("POST", tokens[0], `{}`, 409)
	call("PUT", tokens[1], `{"invite_code":"`+strings.Repeat("0", 32)+`"}`, 403)
	joined := call("PUT", tokens[1], `{"invite_code":"`+code+`"}`, 200)["party"].(map[string]any)
	if joined["id"] != first["id"] || len(joined["members"].([]any)) != 2 {
		t.Fatal(joined)
	}
	call("POST", tokens[1], `{}`, 409)
	second := call("POST", tokens[2], `{}`, 200)["party"].(map[string]any)
	// Two replicas cannot place one account in two parties.
	results := make(chan int, 2)
	for _, invite := range []string{code, second["invite_code"].(string)} {
		go func(invite string) {
			r := httptest.NewRequest("PUT", "https://backend.test/v1/party", strings.NewReader(`{"invite_code":"`+invite+`"}`))
			r.Header.Set("Authorization", "Bearer "+tokens[3])
			w := httptest.NewRecorder()
			service.ServeHTTP(w, r)
			results <- w.Code
		}(invite)
	}
	accepted, rejected := 0, 0
	for range 2 {
		switch <-results {
		case 200:
			accepted++
		case 409:
			rejected++
		}
	}
	if accepted != 1 || rejected != 1 {
		t.Fatal("concurrent party membership escaped uniqueness")
	}
	call("DELETE", tokens[1], `{}`, 200)
	if call("GET", tokens[1], "", 200)["party"] != nil {
		t.Fatal("leave retained membership")
	}
	call("DELETE", tokens[0], `{}`, 200)
	call("PUT", tokens[1], `{"invite_code":"`+code+`"}`, 403)
	if call("GET", tokens[0], "", 200)["party"] != nil {
		t.Fatal("leader leave retained party")
	}
	// Exercise the engine's 64-player ceiling using only this private test database.
	var count int
	if err := db.QueryRow("SELECT count(*) FROM backend_party_members WHERE party_id=$1", second["id"]).Scan(&count); err != nil {
		t.Fatal(err)
	}
	if _, err := db.Exec("INSERT INTO backend_profiles(player_id) SELECT n::text FROM generate_series(2000,$1) n", 2063-count); err != nil {
		t.Fatal(err)
	}
	if _, err := db.Exec("INSERT INTO backend_party_members(player_id,party_id) SELECT n::text,$1 FROM generate_series(2000,$2) n", second["id"], 2063-count); err != nil {
		t.Fatal(err)
	}
	call("PUT", tokens[0], `{"invite_code":"`+second["invite_code"].(string)+`"}`, 409)
	if len(call("GET", tokens[2], "", 200)["party"].(map[string]any)["members"].([]any)) != 64 {
		t.Fatal("party capacity changed")
	}
	call("POST", tokens[0], `{"leader":"1003"}`, 400)
	// Reload through another pool: membership belongs to the shared database.
	other, err := openBackendDB(context.Background(), os.Getenv("BACKEND_TEST_DATABASE"))
	if err != nil {
		t.Fatal(err)
	}
	defer other.Close()
	service.db = other
	if call("GET", tokens[2], "", 200)["party"].(map[string]any)["id"] != second["id"] {
		t.Fatal("party did not persist")
	}
}

func TestBackendQueueRecovery(t *testing.T) {
	db, err := openBackendDB(context.Background(), os.Getenv("BACKEND_TEST_DATABASE"))
	if err != nil {
		t.Fatal(err)
	}
	defer db.Close()
	udp, err := net.ListenPacket("udp", "127.0.0.1:0")
	if err != nil {
		t.Fatal(err)
	}
	defer udp.Close()
	var matchID atomic.Value
	matchID.Store("")
	var ready atomic.Bool
	udpDone := make(chan struct{})
	go func() {
		defer close(udpDone)
		buffer := make([]byte, 4096)
		for {
			n, peer, err := udp.ReadFrom(buffer)
			if err != nil {
				return
			}
			fields := strings.Fields(string(buffer[:n]))
			if len(fields) != 2 {
				continue
			}
			id := ""
			if ready.Load() {
				id = matchID.Load().(string)
			}
			udp.WriteTo([]byte("\xff\xff\xff\xffinfoResponse\n\\challenge\\"+fields[1]+"\\mapname\\two_lane\\as_match\\"+id), peer)
		}
	}()
	defer func() { udp.Close(); <-udpDone }()
	var allocations atomic.Int32
	var captured struct {
		Match   contracts.MatchSpec `json:"match"`
		JoinKey string              `json:"join_key"`
		Token   string              `json:"token"`
	}
	var lock sync.Mutex
	api := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/steam" {
			ids := map[string]string{"2100": "3001", "2200": "3002"}
			fmt.Fprintf(w, `{"response":{"params":{"result":"OK","steamid":"%s"}}}`, ids[r.URL.Query().Get("ticket")])
			return
		}
		if r.Header.Get("Authorization") != "Bearer private-kube-token" {
			http.Error(w, "not authorized", 401)
			return
		}
		lock.Lock()
		defer lock.Unlock()
		if r.Method == "GET" && r.URL.Path == "/apis/agones.dev/v1/namespaces/backend-test/gameservers" {
			items := []any{}
			if allocations.Load() > 0 {
				if r.URL.Query().Get("labelSelector") != "aftershock.dev/match="+captured.Match.ID {
					t.Error("allocation recovery omitted exact match label")
				}
				items = append(items, map[string]any{"metadata": map[string]any{"name": "owned-game", "labels": map[string]string{"aftershock.dev/match": captured.Match.ID}}, "status": map[string]any{
					"state": "Allocated", "address": "127.0.0.1", "ports": []any{map[string]any{"name": "game", "port": udp.LocalAddr().(*net.UDPAddr).Port}}}})
			}
			json.NewEncoder(w).Encode(map[string]any{"items": items})
			return
		}
		if r.Method == "POST" && r.URL.Path == "/apis/allocation.agones.dev/v1/namespaces/backend-test/gameserverallocations" {
			allocations.Add(1)
			var request struct {
				Spec struct {
					Metadata struct{ Annotations, Labels map[string]string }
				}
			}
			if json.NewDecoder(r.Body).Decode(&request) != nil || json.Unmarshal([]byte(request.Spec.Metadata.Annotations["aftershock.dev/match"]), &captured) != nil || captured.Match.Validate() != nil ||
				request.Spec.Metadata.Labels["aftershock.dev/match"] != captured.Match.ID {
				t.Error("invalid allocation contract")
			}
			matchID.Store(captured.Match.ID)
			// The allocation committed, but its response was lost. Only recovery may follow.
			http.Error(w, "lost allocation response", 503)
			return
		}
		t.Error("unexpected cluster API", r.Method, r.URL.Path)
		http.NotFound(w, r)
	}))
	defer api.Close()
	service := &backendService{db: db, steamURL: api.URL + "/steam", steamKey: "private-test-key", appID: "12345", client: api.Client(),
		kubeURL: api.URL, kubeToken: "private-kube-token", namespace: "backend-test", fleet: "aftershock", mapName: "two_lane", matchMinutes: 1}
	call := func(method, path, token, body string, want int) map[string]any {
		t.Helper()
		r := httptest.NewRequest(method, "https://backend.test"+path, strings.NewReader(body))
		if token != "" {
			r.Header.Set("Authorization", "Bearer "+token)
		}
		w := httptest.NewRecorder()
		service.ServeHTTP(w, r)
		if w.Code != want {
			t.Fatalf("%s %s: got %d want %d: %s", method, path, w.Code, want, w.Body.String())
		}
		var result map[string]any
		if json.Unmarshal(w.Body.Bytes(), &result) != nil {
			t.Fatal("invalid JSON response")
		}
		return result
	}
	one := call("POST", "/v1/login", "", `{"ticket":"2100"}`, 200)["token"].(string)
	two := call("POST", "/v1/login", "", `{"ticket":"2200"}`, 200)["token"].(string)
	party := call("POST", "/v1/party", one, `{}`, 200)["party"].(map[string]any)
	call("PUT", "/v1/party", two, `{"invite_code":"`+party["invite_code"].(string)+`"}`, 200)
	call("POST", "/v1/queue", two, `{"map":"two_lane","mode":0}`, 403)
	call("POST", "/v1/queue", one, `{"map":"unknown","mode":0}`, 400)
	call("POST", "/v1/queue", one, `{"map":"two_lane"}`, 400)
	call("POST", "/v1/queue", one, `{"map":"two_lane","mode":0}`, 503)
	call("DELETE", "/v1/party", one, `{}`, 409)
	call("DELETE", "/v1/party", two, `{}`, 409)
	pending := call("GET", "/v1/queue", one, "", 202)
	if pending["state"] != "starting" {
		t.Fatal("allocation bypassed authentication readiness")
	}
	ready.Store(true)
	first := call("GET", "/v1/queue", one, "", 200)
	second := call("GET", "/v1/queue", two, "", 200)
	lock.Lock()
	key, err := hex.DecodeString(captured.JoinKey)
	expected := captured.Match
	lock.Unlock()
	if err != nil || len(key) != 32 || len(expected.ExpectedPlayers) != 2 {
		t.Fatal("missing private signing material")
	}
	for index, assignment := range []map[string]any{first, second} {
		claims, err := contracts.VerifyJoin(key, assignment["ticket"].(string), expected.ID, time.Now().Unix())
		if err != nil || claims.PlayerID != []string{"3001", "3002"}[index] || assignment["address"] != udp.LocalAddr().String() {
			t.Fatal("ticket identity/address mismatch", err)
		}
	}
	if allocations.Load() != 1 {
		t.Fatal("lost response allocated a second pod")
	}
	// Reload another replica and repeat POST: same assignment, newly issued nonce.
	other, err := openBackendDB(context.Background(), os.Getenv("BACKEND_TEST_DATABASE"))
	if err != nil {
		t.Fatal(err)
	}
	defer other.Close()
	service.db = other
	retry := call("POST", "/v1/queue", one, `{"map":"two_lane","mode":0}`, 200)
	if retry["match_id"] != first["match_id"] || retry["ticket"] == first["ticket"] || allocations.Load() != 1 {
		t.Fatal("restart lost assignment or reused ticket")
	}
}
