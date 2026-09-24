// Owned #30 acceptance fixture only. Not part of the production image.
package main

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"sync"
	"time"
)

func main() {
	var err error
	if len(os.Args) == 2 && os.Args[1] == "inspect" {
		files := map[string][]byte{}
		for _, name := range []string{"baseoa/games.log", "engine.done", "pending.json", "ack.json", "results.done"} {
			file, openErr := os.Open(filepath.Join("/home/match", name))
			if os.IsNotExist(openErr) {
				continue
			}
			if openErr != nil {
				err = openErr
				break
			}
			data, readErr := io.ReadAll(io.LimitReader(file, 131073))
			file.Close()
			if readErr != nil || len(data) > 131072 {
				err = errors.New("fixture inspection exceeds bound")
				break
			}
			files[name] = data
		}
		if err == nil {
			err = json.NewEncoder(os.Stdout).Encode(files)
		}
	} else if os.Getenv("FIXTURE_MODE") == "coordinator" {
		err = coordinate()
	} else {
		err = produce()
	}
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}

func coordinate() error {
	var mu sync.Mutex
	ready := map[string]bool{}
	ended := map[string]int64{}
	acked := map[string]int64{}
	gate := make(chan struct{})
	retire := make(chan struct{})
	released := false
	retired := false
	mux := http.NewServeMux()
	mux.HandleFunc("/status", func(w http.ResponseWriter, r *http.Request) {
		mu.Lock()
		defer mu.Unlock()
		_ = json.NewEncoder(w).Encode(map[string]any{"ready": len(ready), "released": released, "ended": ended, "acked": acked})
	})
	mux.HandleFunc("/ended", func(w http.ResponseWriter, r *http.Request) {
		mu.Lock()
		defer mu.Unlock()
		id := r.URL.Query().Get("match")
		if !released || !ready[id] {
			w.WriteHeader(http.StatusConflict)
			return
		}
		ended[id] = time.Now().UnixMilli()
	})
	mux.HandleFunc("/ack", func(w http.ResponseWriter, r *http.Request) {
		id := r.URL.Query().Get("match")
		mu.Lock()
		if !released || !ready[id] {
			mu.Unlock()
			w.WriteHeader(http.StatusConflict)
			return
		}
		acked[id] = time.Now().UnixMilli()
		mu.Unlock()
		select {
		case <-retire:
			_, _ = w.Write([]byte("retire"))
		case <-r.Context().Done():
		}
	})
	mux.HandleFunc("/retire", func(w http.ResponseWriter, r *http.Request) {
		mu.Lock()
		defer mu.Unlock()
		if r.Method != http.MethodPost || len(acked) != 100 || retired {
			w.WriteHeader(http.StatusConflict)
			return
		}
		retired = true
		close(retire)
	})
	mux.HandleFunc("/release", func(w http.ResponseWriter, r *http.Request) {
		mu.Lock()
		defer mu.Unlock()
		if r.Method != http.MethodPost || len(ready) != 100 || released {
			w.WriteHeader(http.StatusConflict)
			return
		}
		released = true
		close(gate)
		_, _ = w.Write([]byte("released"))
	})
	mux.HandleFunc("/wait", func(w http.ResponseWriter, r *http.Request) {
		id := r.URL.Query().Get("match")
		if len(id) < 1 || len(id) > 64 {
			w.WriteHeader(http.StatusBadRequest)
			return
		}
		mu.Lock()
		ready[id] = true
		mu.Unlock()
		select {
		case <-gate:
			_, _ = w.Write([]byte("finish"))
		case <-r.Context().Done():
		}
	})
	server := &http.Server{Addr: ":8080", Handler: mux, ReadHeaderTimeout: 5 * time.Second}
	return server.ListenAndServe()
}

func produce() error {
	client := &http.Client{Timeout: 5 * time.Second}
	sdkURL := "http://127.0.0.1:" + os.Getenv("AGONES_SDK_HTTP_PORT")
	sdk := func(method, path string, out any) error {
		request, err := http.NewRequest(method, sdkURL+path, bytes.NewBufferString("{}"))
		if err != nil {
			return err
		}
		request.Header.Set("Content-Type", "application/json")
		response, err := client.Do(request)
		if err != nil {
			return err
		}
		defer response.Body.Close()
		if response.StatusCode != 200 {
			return errors.New("fixture SDK request failed")
		}
		if out != nil {
			return json.NewDecoder(io.LimitReader(response.Body, 65536)).Decode(out)
		}
		return nil
	}
	deadline := time.Now().Add(10 * time.Minute)
	for sdk("POST", "/ready", nil) != nil {
		if time.Now().After(deadline) {
			return errors.New("fixture SDK readiness timed out")
		}
		time.Sleep(time.Second)
	}
	stop := make(chan struct{})
	defer close(stop)
	go func() {
		for {
			_ = sdk("POST", "/health", nil)
			select {
			case <-stop:
				return
			case <-time.After(2 * time.Second):
			}
		}
	}()
	var allocation struct {
		ID       string   `json:"id"`
		Expected []string `json:"expected_players"`
	}
	var annotation string
	for {
		var gs struct {
			ObjectMeta struct {
				Annotations map[string]string `json:"annotations"`
			} `json:"object_meta"`
			Status struct {
				State string `json:"state"`
			} `json:"status"`
		}
		if sdk("GET", "/gameserver", &gs) == nil && gs.Status.State == "Allocated" {
			annotation = gs.ObjectMeta.Annotations["aftershock.dev/match"]
			if json.Unmarshal([]byte(annotation), &allocation) != nil || allocation.ID == "" || len(allocation.Expected) != 1 {
				return errors.New("fixture allocation missing signed player contract")
			}
			break
		}
		if time.Now().After(deadline) {
			return errors.New("fixture allocation timed out")
		}
		time.Sleep(time.Second)
	}
	home := "/home/match"
	if err := os.MkdirAll(filepath.Join(home, "aftershock"), 0700); err != nil {
		return err
	}
	// Rename prevents the real shipper from observing a partial specification.
	if err := os.WriteFile(filepath.Join(home, "match.json.next"), []byte(annotation), 0600); err != nil {
		return err
	}
	if err := os.Rename(filepath.Join(home, "match.json.next"), filepath.Join(home, "match.json")); err != nil {
		return err
	}
	log, err := os.OpenFile(filepath.Join(home, "aftershock/games.log"), os.O_CREATE|os.O_WRONLY|os.O_EXCL, 0600)
	if err != nil {
		return err
	}
	defer log.Close()
	if _, err = fmt.Fprintf(log, "  0:00 InitGame: \\mapname\\two_lane\n  0:00 ClientIdentity: 0 %s\n  0:00 ClientBegin: 0\n", allocation.Expected[0]); err != nil {
		return err
	}
	if err = log.Sync(); err != nil {
		return err
	}
	// Hold every producer at the same barrier until all 100 have allocated.
	waiter := &http.Client{Timeout: 10 * time.Minute}
	response, err := waiter.Get("http://burst-coordinator:8080/wait?match=" + allocation.ID)
	if err != nil {
		return err
	}
	response.Body.Close()
	if response.StatusCode != 200 {
		return errors.New("fixture barrier rejected")
	}
	if _, err = fmt.Fprint(log, "  1:00 Exit: Timelimit hit.\n  1:00 score: 1 ping: 0 client: 0 fixture\n  1:00 ShutdownGame:\n"); err != nil {
		return err
	}
	if err = log.Sync(); err != nil {
		return err
	}
	if err = os.WriteFile(filepath.Join(home, "engine.done"), []byte("{}"), 0600); err != nil {
		return err
	}
	response, err = client.Get("http://burst-coordinator:8080/ended?match=" + allocation.ID)
	if err != nil {
		return err
	}
	response.Body.Close()
	if response.StatusCode != 200 {
		return errors.New("fixture completion not recorded")
	}
	for {
		if _, err = os.Stat(filepath.Join(home, "results.done")); err == nil {
			fmt.Printf("fixture acknowledged %s\n", allocation.ID)
			// Hold retirement until the write-burst latency window is measured,
			// so replacement-pod startup is not confused with ingest pressure.
			response, err := waiter.Get("http://burst-coordinator:8080/ack?match=" + allocation.ID)
			if err != nil {
				return err
			}
			response.Body.Close()
			if response.StatusCode != 200 {
				return errors.New("fixture retirement not released")
			}
			return sdk("POST", "/shutdown", nil)
		}
		if time.Now().After(deadline) {
			return errors.New("fixture final acknowledgement timed out")
		}
		time.Sleep(100 * time.Millisecond)
	}
}
