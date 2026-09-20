package main

import (
	"bufio"
	"context"
	"crypto/tls"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/metadata"
	"google.golang.org/protobuf/types/known/emptypb"
	"google.golang.org/protobuf/types/known/structpb"
)

// Shared emptyDir files are private to the non-root match containers. Rename and
// fsync keep a restart from advancing the cursor past an unacknowledged batch.
func writeJSON(path string, value any) error {
	data, err := json.Marshal(value)
	if err != nil {
		return err
	}
	f, err := os.OpenFile(path+".tmp", os.O_CREATE|os.O_TRUNC|os.O_WRONLY, 0600)
	if err != nil {
		return err
	}
	_, err = f.Write(data)
	if err == nil {
		err = f.Sync()
	}
	closeErr := f.Close()
	if err == nil {
		err = closeErr
	}
	if err != nil {
		return err
	}
	if err = os.Rename(path+".tmp", path); err != nil {
		return err
	}
	dir, err := os.Open(filepath.Dir(path))
	if err != nil {
		return err
	}
	defer dir.Close()
	return dir.Sync()
}
func readJSON(path string, value any) error {
	f, err := os.Open(path)
	if err != nil {
		return err
	}
	defer f.Close()
	data, err := io.ReadAll(io.LimitReader(f, 65537))
	if err != nil {
		return err
	}
	return strictJSON(data, value)
}
func exists(path string) bool { _, err := os.Stat(path); return err == nil }
func pause(ctx context.Context, d time.Duration) error {
	select {
	case <-ctx.Done():
		return ctx.Err()
	case <-time.After(d):
		return nil
	}
}

type cursor struct {
	End        int64      `json:"end"`
	Checkpoint checkpoint `json:"checkpoint"`
	Final      bool       `json:"final"`
}

func ship(ctx context.Context, home, game, address string, dev bool) error {
	if !identifier.MatchString(game) {
		return errors.New("invalid game directory")
	}
	var s spec
	for !exists(filepath.Join(home, "match.json")) {
		if err := pause(ctx, time.Second); err != nil {
			return err
		}
	}
	if err := readJSON(filepath.Join(home, "match.json"), &s); err != nil {
		return err
	}
	data, _ := json.Marshal(s)
	if _, err := decodeSpec(data); err != nil {
		return err
	}
	transport := credentials.NewTLS(&tls.Config{MinVersion: tls.VersionTLS12})
	if dev {
		transport = insecure.NewCredentials()
	}
	connection, err := grpc.NewClient(address, grpc.WithTransportCredentials(transport), grpc.WithDefaultCallOptions(grpc.MaxCallSendMsgSize(131072)))
	if err != nil {
		return err
	}
	defer connection.Close()
	var ack cursor
	ackPath, pendingPath := filepath.Join(home, "ack.json"), filepath.Join(home, "pending.json")
	if err = readJSON(ackPath, &ack); err != nil && !os.IsNotExist(err) {
		return err
	}
	for !ack.Final {
		var b batch
		if err = readJSON(pendingPath, &b); err != nil {
			if !os.IsNotExist(err) {
				return err
			}
			b = batch{Version: 1, Match: s.ID, Start: ack.End, End: ack.End, Checkpoint: ack.Checkpoint}
			f, err := os.Open(filepath.Join(home, game, "games.log"))
			if os.IsNotExist(err) && exists(filepath.Join(home, "engine.done")) {
				b.Final = true
			} else {
				if os.IsNotExist(err) {
					if err = pause(ctx, time.Second); err != nil {
						return err
					}
					continue
				}
				if err != nil {
					return err
				}
				if _, err = f.Seek(ack.End, io.SeekStart); err != nil {
					f.Close()
					return err
				}
				reader := bufio.NewReaderSize(f, 4096)
				eof := false
				for len(b.Events) < 64 && b.End-b.Start < 8192 {
					line, readErr := reader.ReadSlice('\n')
					if readErr == io.EOF {
						eof = true
						if len(line) > 0 && exists(filepath.Join(home, "engine.done")) {
							f.Close()
							return errors.New("incomplete final log line")
						}
						break
					}
					if readErr != nil {
						f.Close()
						return fmt.Errorf("game log line exceeds bound or cannot be read: %w", readErr)
					}
					b.End += int64(len(line))
					value := strings.TrimSuffix(string(line), "\n")
					b.Events = append(b.Events, value)
					b.Checkpoint.add(value)
				}
				// The engine may append between EOF and its done marker. Only seal
				// a cursor that covers the final closed file size.
				if eof && exists(filepath.Join(home, "engine.done")) {
					info, statErr := f.Stat()
					if statErr != nil {
						f.Close()
						return statErr
					}
					b.Final = b.End == info.Size()
				}
				f.Close()
			}
			if b.End == b.Start && !b.Final {
				if err = pause(ctx, 2*time.Second); err != nil {
					return err
				}
				continue
			}
			if err = writeJSON(pendingPath, b); err != nil {
				return err
			}
		}
		payload, err := json.Marshal(b)
		if err != nil {
			return err
		}
		request := new(structpb.Struct)
		if err = request.UnmarshalJSON(payload); err != nil {
			return err
		}
		call, cancel := context.WithTimeout(metadata.AppendToOutgoingContext(ctx, "authorization", "Bearer "+s.Token), 3*time.Second)
		err = connection.Invoke(call, "/aftershock.match.v1.Ingest/Append", request, &emptypb.Empty{})
		cancel()
		if err != nil {
			fmt.Println("ingest unavailable; retaining pending batch")
			if err = pause(ctx, time.Second); err != nil {
				return err
			}
			continue
		}
		ack = cursor{End: b.End, Checkpoint: b.Checkpoint, Final: b.Final}
		if err = writeJSON(ackPath, ack); err != nil {
			return err
		}
		if err = os.Remove(pendingPath); err != nil {
			return err
		}
	}
	if err = writeJSON(filepath.Join(home, "results.done"), map[string]any{"match": s.ID, "end": ack.End}); err != nil {
		return err
	}
	fmt.Printf("results acknowledged match=%s end=%d\n", s.ID, ack.End)
	return nil
}
