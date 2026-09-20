package main

import (
	"bufio"
	"context"
	"crypto/sha256"
	"crypto/subtle"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"path/filepath"
	"strings"
	"sync"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/status"
	"google.golang.org/protobuf/types/known/emptypb"
	"google.golang.org/protobuf/types/known/structpb"
)

type batch struct {
	Version    int        `json:"version"`
	Match      string     `json:"match"`
	Start      int64      `json:"start"`
	End        int64      `json:"end"`
	Events     []string   `json:"events"`
	Checkpoint checkpoint `json:"checkpoint"`
	Final      bool       `json:"final"`
}
type ingest struct {
	mu      sync.Mutex
	file    *os.File
	tokens  map[string]string
	offsets map[string]int64
	closed  map[string]bool
	digests map[string][32]byte
	failed  bool
}

func batchKey(b batch) string { return fmt.Sprintf("%s:%d:%d:%t", b.Match, b.Start, b.End, b.Final) }
func newIngest(path string, tokens map[string]string) (*ingest, error) {
	if err := os.MkdirAll(filepath.Dir(path), 0700); err != nil {
		return nil, err
	}
	f, err := os.OpenFile(path, os.O_CREATE|os.O_RDWR|os.O_APPEND, 0600)
	if err != nil {
		return nil, err
	}
	s := &ingest{file: f, tokens: tokens, offsets: map[string]int64{}, closed: map[string]bool{}, digests: map[string][32]byte{}}
	info, err := f.Stat()
	if err != nil {
		f.Close()
		return nil, err
	}
	if info.Size() > 0 {
		last := make([]byte, 1)
		if _, err = f.ReadAt(last, info.Size()-1); err != nil || last[0] != '\n' {
			f.Close()
			return nil, errors.New("incomplete durable record; explicit recovery required")
		}
	}
	if _, err = f.Seek(0, io.SeekStart); err != nil {
		f.Close()
		return nil, err
	}
	scan := bufio.NewScanner(f)
	scan.Buffer(make([]byte, 4096), 131072)
	for scan.Scan() {
		var b batch
		if err := json.Unmarshal(scan.Bytes(), &b); err != nil {
			f.Close()
			return nil, err
		}
		data, _ := json.Marshal(b)
		s.offsets[b.Match] = b.End
		s.closed[b.Match] = b.Final
		s.digests[batchKey(b)] = sha256.Sum256(data)
	}
	if err := scan.Err(); err != nil {
		f.Close()
		return nil, err
	}
	return s, nil
}
func (s *ingest) accept(b batch, token string) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	expected, ok := s.tokens[b.Match]
	if !ok || subtle.ConstantTimeCompare([]byte(token), []byte(expected)) != 1 {
		return status.Error(codes.Unauthenticated, "invalid allocation token")
	}
	data, err := json.Marshal(b)
	if err != nil {
		return err
	}
	if b.Version != 1 || !identifier.MatchString(b.Match) || b.Start < 0 || b.End < b.Start || b.End > 1<<33 || len(data) > 65536 || len(b.Events) > 256 || (b.Start == b.End && !b.Final) {
		return status.Error(codes.InvalidArgument, "invalid batch")
	}
	bytes := int64(0)
	for _, line := range b.Events {
		if strings.ContainsAny(line, "\r\n") {
			return status.Error(codes.InvalidArgument, "embedded event newline")
		}
		bytes += int64(len(line) + 1)
	}
	if b.End-b.Start != bytes {
		return status.Error(codes.InvalidArgument, "offset does not cover events")
	}
	digest := sha256.Sum256(data)
	if previous, ok := s.digests[batchKey(b)]; ok {
		if previous != digest {
			return status.Error(codes.AlreadyExists, "different retry payload")
		}
		return nil
	}
	if s.closed[b.Match] {
		return status.Error(codes.FailedPrecondition, "match stream is already final")
	}
	if s.failed {
		return status.Error(codes.Unavailable, "durable log needs recovery")
	}
	if b.Start != s.offsets[b.Match] {
		return status.Error(codes.FailedPrecondition, "noncontiguous event offsets")
	}
	// ponytail: the development stub retains per-batch digests in memory; #30 owns
	// durable retention/scaling. Fsync before ACK, never in the game loop.
	if _, err = s.file.Write(append(data, '\n')); err == nil {
		err = s.file.Sync()
	}
	if err != nil {
		s.failed = true
		return status.Error(codes.Unavailable, "durable append failed")
	}
	s.offsets[b.Match] = b.End
	s.closed[b.Match] = b.Final
	s.digests[batchKey(b)] = digest
	fmt.Printf("ingest match=%s end=%d events=%d final=%t\n", b.Match, b.End, len(b.Events), b.Final)
	return nil
}

type ingestService interface {
	Append(context.Context, *structpb.Struct) (*emptypb.Empty, error)
}

func (s *ingest) Append(ctx context.Context, input *structpb.Struct) (*emptypb.Empty, error) {
	data, err := input.MarshalJSON()
	if err != nil {
		return nil, status.Error(codes.InvalidArgument, "invalid batch JSON")
	}
	var b batch
	if err = strictJSON(data, &b); err != nil {
		return nil, status.Error(codes.InvalidArgument, "invalid batch fields")
	}
	md, _ := metadata.FromIncomingContext(ctx)
	auth := md.Get("authorization")
	if len(auth) != 1 || !strings.HasPrefix(auth[0], "Bearer ") {
		return nil, status.Error(codes.Unauthenticated, "allocation token required")
	}
	if err = s.accept(b, strings.TrimPrefix(auth[0], "Bearer ")); err != nil {
		return nil, err
	}
	return &emptypb.Empty{}, nil
}
func serveIngest(ctx context.Context, address, path string, tokens map[string]string) error {
	s, err := newIngest(path, tokens)
	if err != nil {
		return err
	}
	defer s.file.Close()
	listener, err := net.Listen("tcp", address)
	if err != nil {
		return err
	}
	server := grpc.NewServer(grpc.MaxRecvMsgSize(131072), grpc.MaxConcurrentStreams(32))
	server.RegisterService(&grpc.ServiceDesc{ServiceName: "aftershock.match.v1.Ingest", HandlerType: (*ingestService)(nil), Methods: []grpc.MethodDesc{{MethodName: "Append", Handler: func(srv any, ctx context.Context, decode func(any) error, interceptor grpc.UnaryServerInterceptor) (any, error) {
		in := new(structpb.Struct)
		if err := decode(in); err != nil {
			return nil, err
		}
		call := func(ctx context.Context, request any) (any, error) {
			return srv.(ingestService).Append(ctx, request.(*structpb.Struct))
		}
		if interceptor == nil {
			return call(ctx, in)
		}
		return interceptor(ctx, in, &grpc.UnaryServerInfo{Server: srv, FullMethod: "/aftershock.match.v1.Ingest/Append"}, call)
	}}}}, s)
	go func() { <-ctx.Done(); server.Stop() }()
	err = server.Serve(listener)
	if errors.Is(err, grpc.ErrServerStopped) {
		return nil
	}
	return err
}
