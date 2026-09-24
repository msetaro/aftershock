package main

import (
	"context"
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"syscall"
	"time"
)

func env(name, fallback string) string {
	if value := os.Getenv(name); value != "" {
		return value
	}
	return fallback
}
func randomID() string {
	var b [16]byte
	if _, err := rand.Read(b[:]); err != nil {
		panic(err)
	}
	return hex.EncodeToString(b[:])
}
func probeMatch(address, mapName, matchID string) error {
	connection, err := net.DialTimeout("udp", address, time.Second)
	if err != nil {
		return err
	}
	defer connection.Close()
	connection.SetDeadline(time.Now().Add(time.Second))
	nonce := randomID()
	if _, err = connection.Write([]byte("\xff\xff\xff\xffgetinfo " + nonce + "\n")); err != nil {
		return err
	}
	response := make([]byte, 4096)
	n, err := connection.Read(response)
	if err != nil {
		return err
	}
	value := string(response[:n])
	if !strings.HasPrefix(value, "\xff\xff\xff\xffinfoResponse\n") || !strings.Contains(value, "\\challenge\\"+nonce) || !strings.Contains(value, "\\mapname\\"+mapName+"\\") {
		return errors.New("server has not loaded the requested map")
	}
	if matchID != "" && !strings.Contains(value+"\\", "\\as_match\\"+matchID+"\\") {
		return errors.New("server has not loaded the requested join configuration")
	}
	return nil
}
func probe(address, mapName string) error { return probeMatch(address, mapName, "") }
func sdk(ctx context.Context, method, path string, out any) error {
	port, err := strconv.Atoi(env("AGONES_SDK_HTTP_PORT", "9358"))
	if err != nil || port < 1024 || port > 65535 {
		return errors.New("invalid SDK port")
	}
	call, cancel := context.WithTimeout(ctx, 2*time.Second)
	defer cancel()
	request, err := http.NewRequestWithContext(call, method, "http://127.0.0.1:"+strconv.Itoa(port)+path, strings.NewReader("{}"))
	if err != nil {
		return err
	}
	request.Header.Set("Content-Type", "application/json")
	response, err := http.DefaultClient.Do(request)
	if err != nil {
		return err
	}
	defer response.Body.Close()
	if response.StatusCode != 200 {
		return fmt.Errorf("SDK %s returned %d", path, response.StatusCode)
	}
	if out != nil {
		return json.NewDecoder(io.LimitReader(response.Body, 65536)).Decode(out)
	}
	return nil
}
func runServer(ctx context.Context) error {
	home, content, game := env("MATCH_HOME", "/home/match"), env("MATCH_CONTENT", "/content"), env("MATCH_GAME", "aftershock")
	if !identifier.MatchString(game) {
		return errors.New("invalid game directory")
	}
	if err := os.MkdirAll(filepath.Join(home, game), 0700); err != nil {
		return err
	}
	// Each pod is one match. Refuse accidental reuse of its durable cursor/log state.
	if exists(filepath.Join(home, "match.json")) || exists(filepath.Join(home, "engine.done")) {
		return errors.New("match home was already used")
	}
	agones := os.Getenv("AGONES_SDK_HTTP_PORT") != ""
	var s spec
	var err error
	if agones {
		s = spec{ID: "warm-" + randomID(), Map: env("MATCH_MAP", "two_lane"), Mode: 0, FragLimit: 1, TimeLimit: 1, Players: 64, Password: randomID(), Token: randomID()}
	} else {
		data := []byte(os.Getenv("MATCH_SPEC"))
		if path := os.Getenv("MATCH_SPEC_FILE"); path != "" {
			f, e := os.Open(path)
			if e != nil {
				return e
			}
			data, e = io.ReadAll(io.LimitReader(f, 65537))
			f.Close()
			if e != nil {
				return e
			}
		}
		s, err = decodeSpec(data)
		if err != nil {
			return err
		}
	}
	port, err := strconv.Atoi(env("MATCH_PORT", "27960"))
	if err != nil {
		return err
	}
	args, err := serverArgs(s, content, home, game, port, agones)
	if err != nil {
		return err
	}
	if !agones {
		if err = writeJoinConfig(home, s); err != nil {
			return err
		}
		if err = writeJSON(filepath.Join(home, "match.json"), s); err != nil {
			return err
		}
	}
	command := exec.Command(env("MATCH_SERVER", "/app/server"), args...)
	stdin, err := command.StdinPipe()
	if err != nil {
		return err
	}
	log, err := os.OpenFile(filepath.Join(home, "server.log"), os.O_CREATE|os.O_WRONLY|os.O_EXCL, 0600)
	if err != nil {
		return err
	}
	defer log.Close()
	command.Stdout = io.MultiWriter(os.Stdout, log)
	command.Stderr = command.Stdout
	command.SysProcAttr = &syscall.SysProcAttr{Setpgid: true}
	if err = command.Start(); err != nil {
		return err
	}
	done := make(chan error, 1)
	go func() { done <- command.Wait() }()
	stopped := false
	defer func() {
		if !stopped {
			command.Process.Signal(syscall.SIGTERM)
			select {
			case <-done:
			case <-time.After(5 * time.Second):
				command.Process.Kill()
				<-done
			}
		}
	}()
	address := "127.0.0.1:" + strconv.Itoa(port)
	deadline := time.Now().Add(time.Minute)
	ready, allocated, configured := false, !agones, !agones
	var processError error
	for {
		select {
		case processError = <-done:
			stopped = true
			goto ended
		case <-ctx.Done():
			return ctx.Err()
		case <-time.After(time.Second):
		}
		if exists(filepath.Join(home, "drain.request")) {
			// Both preStop hooks keep the controller and shipper alive while the
			// engine closes its log. Only the shipper publishes results.done.
			if _, err = io.WriteString(stdin, "quit\n"); err != nil {
				_ = command.Process.Signal(syscall.SIGTERM)
			}
			select {
			case processError = <-done:
			case <-time.After(5 * time.Second):
				_ = command.Process.Signal(syscall.SIGTERM)
				select {
				case processError = <-done:
				case <-time.After(5 * time.Second):
					_ = command.Process.Kill()
					processError = <-done
				}
			}
			stopped = true
			goto ended
		}
		matchID := ""
		if configured && s.JoinKey != "" {
			matchID = s.ID
		}
		if err = probeMatch(address, s.Map, matchID); err != nil {
			if (!ready || !allocated) && time.Now().After(deadline) {
				return errors.New("server readiness timed out")
			}
			continue
		}
		if agones {
			if err = sdk(ctx, "POST", "/health", nil); err != nil {
				if !ready && time.Now().After(deadline) {
					return err
				}
				continue
			}
			if !ready {
				if err = sdk(ctx, "POST", "/ready", nil); err != nil {
					continue
				}
				ready = true
				fmt.Println("Agones Ready after loaded-map UDP probe")
			}
			if configured && !allocated {
				allocated = true
				fmt.Printf("allocated match=%s\n", s.ID)
			}
			if !configured {
				var gs struct {
					ObjectMeta struct {
						Annotations map[string]string `json:"annotations"`
					} `json:"object_meta"`
					Status struct {
						State string `json:"state"`
					} `json:"status"`
				}
				if err = sdk(ctx, "GET", "/gameserver", &gs); err != nil {
					continue
				}
				if gs.Status.State != "Allocated" {
					continue
				}
				s, err = decodeSpec([]byte(gs.ObjectMeta.Annotations["aftershock.dev/match"]))
				if err != nil {
					return errors.New("allocation needs a valid aftershock.dev/match annotation")
				}
				// Warm Fleet is map-specific. Limits begin only after allocation, never while waiting.
				if s.Map != env("MATCH_MAP", "two_lane") {
					return errors.New("allocated map differs from warm Fleet map")
				}
				if err = writeJoinConfig(home, s); err != nil {
					return err
				}
				if err = writeJSON(filepath.Join(home, "match.json"), s); err != nil {
					return err
				}
				commands := fmt.Sprintf("set g_password \"%s\"\nset sv_maxclients %d\nset g_gametype %d\nset fraglimit %d\nset timelimit %d\nmap %s\n", s.Password, s.Players, s.Mode, s.FragLimit, s.TimeLimit, s.Map)
				if s.JoinKey != "" {
					commands = "joinconfig\n" + commands
				}
				if _, err = io.WriteString(stdin, commands); err != nil {
					return err
				}
				configured = true
				deadline = time.Now().Add(time.Minute)
			}
		} else if !ready {
			ready = true
			fmt.Printf("ready match=%s\n", s.ID)
		}
	}
ended:
	if err = writeJSON(filepath.Join(home, "engine.done"), map[string]any{"clean": processError == nil}); err != nil {
		return err
	}
	if !allocated {
		return errors.New("warm server exited before allocation")
	}
	fmt.Println("engine stopped; waiting for final ingest acknowledgement")
	for !exists(filepath.Join(home, "results.done")) {
		if agones {
			_ = sdk(ctx, "POST", "/health", nil)
		}
		if err = pause(ctx, time.Second); err != nil {
			return err
		}
	}
	if agones {
		deadline = time.Now().Add(time.Minute)
		for {
			err = sdk(ctx, "POST", "/shutdown", nil)
			if err == nil {
				break
			}
			if time.Now().After(deadline) {
				return err
			}
			if err = pause(ctx, time.Second); err != nil {
				return err
			}
		}
	}
	return processError
}
