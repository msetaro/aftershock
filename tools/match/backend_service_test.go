//go:build backend_integration

package main

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"net/http/httptest"
	"os"
	"testing"
)

// The driver supplies a private, disposable real PostgreSQL instance. No mock SQL.
func TestBackendAuthProfile(t *testing.T) {
	dsn := os.Getenv("BACKEND_TEST_DATABASE")
	if dsn == "" {
		t.Fatal("BACKEND_TEST_DATABASE is required; run tests/backend_services.py")
	}
	steam := httptest.NewTLSServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "POST" || r.ParseForm() != nil || r.Form.Get("key") != "private-test-key" || r.Form.Get("appid") != "12345" {
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
