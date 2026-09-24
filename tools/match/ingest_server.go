package main

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"errors"
	"fmt"
	"io"
	"log/slog"
	"net"
	"net/http"
	"net/url"
	"os"
	"regexp"
	"strings"
	"sync/atomic"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials"
	"google.golang.org/grpc/status"
)

func serveDurableIngest(ctx context.Context) error {
	certificate, err := tls.LoadX509KeyPair(os.Getenv("INGEST_TLS_CERT"), os.Getenv("INGEST_TLS_KEY"))
	if err != nil {
		return errors.New("ingest TLS certificate/key required")
	}
	readerFile, err := os.Open(os.Getenv("INGEST_READER_KEY_FILE"))
	if err != nil {
		return errors.New("ingest reader credential required")
	}
	key, err := io.ReadAll(io.LimitReader(readerFile, 130))
	readerFile.Close()
	reader := strings.TrimSpace(string(key))
	if err != nil || len(key) == 130 || len(reader) < 32 || len(reader) > 128 {
		return errors.New("invalid ingest reader credential")
	}
	namespace := os.Getenv("INGEST_NAMESPACE")
	if !regexp.MustCompile(`^[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?$`).MatchString(namespace) {
		return errors.New("ingest namespace required")
	}
	endpoint := env("INGEST_KUBE_URL", "https://kubernetes.default.svc")
	parsed, err := url.Parse(endpoint)
	if err != nil || parsed.Scheme != "https" || parsed.Host == "" || parsed.User != nil || parsed.RawQuery != "" || parsed.Fragment != "" {
		return errors.New("invalid ingest Kubernetes endpoint")
	}
	tokenFile := env("INGEST_KUBE_TOKEN_FILE", "/var/run/secrets/kubernetes.io/serviceaccount/token")
	if _, err := kubeCredential(tokenFile); err != nil {
		return err
	}
	roots, err := x509.SystemCertPool()
	if err != nil || roots == nil {
		roots = x509.NewCertPool()
	}
	ca, err := os.ReadFile(env("INGEST_KUBE_CA_FILE", "/var/run/secrets/kubernetes.io/serviceaccount/ca.crt"))
	if err != nil || len(ca) > 65536 || !roots.AppendCertsFromPEM(ca) {
		return errors.New("ingest Kubernetes trust roots unavailable")
	}
	transport := http.DefaultTransport.(*http.Transport).Clone()
	transport.TLSClientConfig = &tls.Config{MinVersion: tls.VersionTLS12, RootCAs: roots}
	defer transport.CloseIdleConnections()
	cluster := kubernetesClient{url: endpoint, namespace: namespace, tokenFile: tokenFile,
		client: &http.Client{Transport: transport, Timeout: 5 * time.Second}}
	startup, cancel := context.WithTimeout(ctx, 15*time.Second)
	db, err := openResultsDB(startup, os.Getenv("INGEST_DATABASE"))
	cancel()
	if err != nil {
		return err
	}
	defer db.Close()
	service := &durableIngest{db: db, reader: reader, verify: cluster.verifyAllocation}
	var counters [3]struct{ requests, errors, nanos atomic.Uint64 }
	names := []string{"submit", "read", "unsupported"}
	logger := slog.New(slog.NewJSONHandler(os.Stdout, nil))
	capacity := make(chan struct{}, 64)
	intercept := func(ctx context.Context, request any, info *grpc.UnaryServerInfo, call grpc.UnaryHandler) (any, error) {
		select {
		case capacity <- struct{}{}:
			defer func() { <-capacity }()
		default:
			return nil, status.Error(codes.ResourceExhausted, "ingest request capacity reached")
		}
		index := 2
		if info.FullMethod == "/aftershock.match.v1.Ingest/SubmitBatch" {
			index = 0
		} else if info.FullMethod == "/aftershock.match.v1.Ingest/Read" {
			index = 1
		}
		started := time.Now()
		// Bound database work even if a caller omits its own RPC deadline.
		bounded, cancel := context.WithTimeout(ctx, 10*time.Second)
		defer cancel()
		response, err := call(bounded, request)
		duration := time.Since(started)
		counters[index].requests.Add(1)
		counters[index].nanos.Add(uint64(duration.Nanoseconds()))
		if err != nil {
			counters[index].errors.Add(1)
		}
		logger.Info("ingest request", "method", names[index], "code", status.Code(err).String(), "duration_ms", duration.Milliseconds())
		return response, err
	}
	tlsConfig := &tls.Config{MinVersion: tls.VersionTLS12, Certificates: []tls.Certificate{certificate}}
	server := grpc.NewServer(grpc.Creds(credentials.NewTLS(tlsConfig)), grpc.MaxRecvMsgSize(131072),
		grpc.MaxSendMsgSize(65536), grpc.MaxConcurrentStreams(32), grpc.UnaryInterceptor(intercept))
	registerIngest(server, service)
	listener, err := net.Listen("tcp", env("INGEST_LISTEN", ":50051"))
	if err != nil {
		return errors.New("ingest listener unavailable")
	}
	defer listener.Close()
	mux := http.NewServeMux()
	mux.HandleFunc("/healthz", func(w http.ResponseWriter, r *http.Request) {
		check, cancel := context.WithTimeout(r.Context(), time.Second)
		defer cancel()
		if db.PingContext(check) != nil {
			backendJSON(w, 503, map[string]bool{"ready": false})
			return
		}
		backendJSON(w, 200, map[string]bool{"ready": true})
	})
	mux.HandleFunc("/metrics", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/plain; version=0.0.4")
		for i, name := range names {
			fmt.Fprintf(w, "aftershock_ingest_requests_total{method=%q} %d\naftershock_ingest_errors_total{method=%q} %d\naftershock_ingest_seconds_total{method=%q} %.9f\n",
				name, counters[i].requests.Load(), name, counters[i].errors.Load(), name, float64(counters[i].nanos.Load())/1e9)
		}
	})
	health := &http.Server{Addr: env("INGEST_HTTP_LISTEN", ":8444"), Handler: mux, TLSConfig: tlsConfig.Clone(),
		ReadHeaderTimeout: 5 * time.Second, ReadTimeout: 10 * time.Second, WriteTimeout: 10 * time.Second, IdleTimeout: 30 * time.Second, MaxHeaderBytes: 8192}
	failures := make(chan error, 2)
	go func() { failures <- server.Serve(listener) }()
	go func() { failures <- health.ListenAndServeTLS("", "") }()
	select {
	case <-ctx.Done():
	case err = <-failures:
	}
	stopped := make(chan struct{})
	go func() { server.GracefulStop(); close(stopped) }()
	select {
	case <-stopped:
	case <-time.After(10 * time.Second):
		server.Stop()
	}
	shutdown, done := context.WithTimeout(context.Background(), 5*time.Second)
	defer done()
	_ = health.Shutdown(shutdown)
	if err != nil && !errors.Is(err, grpc.ErrServerStopped) && !errors.Is(err, http.ErrServerClosed) {
		return errors.New("ingest listener stopped unexpectedly")
	}
	return nil
}
