// Owned HTTPS authentication fixture; never linked into the shipping match image.
package main

import (
	"crypto/tls"
	"encoding/json"
	"log"
	"net/http"
	"os"
	"time"
)

func main() {
	data, err := os.ReadFile("/fixture/config.json")
	if err != nil {
		log.Fatal("fixture configuration unavailable")
	}
	var config map[string]string
	if json.Unmarshal(data, &config) != nil || len(config) != 5 {
		log.Fatal("invalid fixture configuration")
	}
	player := config["player"]
	delete(config, "player")
	server := &http.Server{Addr: ":8443", ReadHeaderTimeout: 5 * time.Second,
		TLSConfig: &tls.Config{MinVersion: tls.VersionTLS12},
		Handler: http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			query := r.URL.Query()
			if r.Method != "GET" || r.URL.Path != "/authenticate" || len(query) != len(config) {
				w.WriteHeader(http.StatusUnauthorized)
				return
			}
			for key, expected := range config {
				if len(query[key]) != 1 || query.Get(key) != expected {
					w.WriteHeader(http.StatusUnauthorized)
					return
				}
			}
			w.Header().Set("Content-Type", "application/json")
			err := json.NewEncoder(w).Encode(map[string]any{"response": map[string]any{"params": map[string]any{
				"result": "OK", "steamid": player, "publisherbanned": false}}})
			if err != nil {
				log.Print("fixture response failed")
			}
		})}
	log.Fatal(server.ListenAndServeTLS("/fixture/tls.crt", "/fixture/tls.key"))
}
