package main

import (
	"bytes"
	"context"
	"crypto/subtle"
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"net/url"
	"os"
	"strings"
)

type kubernetesClient struct {
	url, tokenFile, namespace string
	client                    *http.Client
}

// The first committed batch binds an allocated pod's private token and allowed
// players to its durable stream. Later retries survive GameServer deletion.
func (k *kubernetesClient) verifyAllocation(ctx context.Context, match, token string) ([]string, error) {
	invalid := errors.New("invalid allocated match identity")
	if !identifier.MatchString(match) || !secret.MatchString(token) {
		return nil, invalid
	}
	var servers struct {
		Items []struct {
			Metadata struct {
				Labels, Annotations map[string]string
			}
			Status struct{ State string }
		}
	}
	path := "/apis/agones.dev/v1/namespaces/" + url.PathEscape(k.namespace) + "/gameservers?labelSelector=" + url.QueryEscape("aftershock.dev/match="+match)
	if err := k.request(ctx, "GET", path, nil, &servers); err != nil {
		return nil, err
	}
	if len(servers.Items) != 1 {
		return nil, invalid
	}
	server := servers.Items[0]
	if server.Status.State != "Allocated" || server.Metadata.Labels["aftershock.dev/match"] != match {
		return nil, invalid
	}
	s, err := decodeSpec([]byte(server.Metadata.Annotations["aftershock.dev/match"]))
	if err != nil || s.ID != match || subtle.ConstantTimeCompare([]byte(s.Token), []byte(token)) != 1 {
		return nil, invalid
	}
	return s.ExpectedPlayers, nil
}

func kubeCredential(path string) (string, error) {
	credential, err := os.Open(path)
	if err != nil {
		return "", errors.New("cluster credential unavailable")
	}
	defer credential.Close()
	data, err := io.ReadAll(io.LimitReader(credential, 16385))
	if err != nil || len(data) > 16384 || strings.TrimSpace(string(data)) == "" {
		return "", errors.New("invalid cluster credential")
	}
	return strings.TrimSpace(string(data)), nil
}

func (k *kubernetesClient) request(ctx context.Context, method, path string, body, out any) error {
	// Projected service-account tokens rotate through an atomic file replacement.
	// Reopen for every request and fail closed rather than retaining expired bytes.
	token, err := kubeCredential(k.tokenFile)
	if err != nil {
		return err
	}
	var reader io.Reader
	if body != nil {
		data, err := json.Marshal(body)
		if err != nil {
			return err
		}
		reader = bytes.NewReader(data)
	}
	request, err := http.NewRequestWithContext(ctx, method, strings.TrimSuffix(k.url, "/")+path, reader)
	if err != nil {
		return errors.New("cluster request unavailable")
	}
	request.Header.Set("Authorization", "Bearer "+token)
	request.Header.Set("Content-Type", "application/json")
	client := *k.client
	client.CheckRedirect = func(*http.Request, []*http.Request) error { return http.ErrUseLastResponse }
	response, err := client.Do(request)
	if err != nil {
		return errors.New("cluster request unavailable")
	}
	defer response.Body.Close()
	if response.StatusCode != 200 && response.StatusCode != 201 {
		return errors.New("cluster request rejected")
	}
	data, err := io.ReadAll(io.LimitReader(response.Body, 65537))
	if err != nil || len(data) > 65536 {
		return errors.New("invalid cluster response")
	}
	return json.Unmarshal(data, out)
}
