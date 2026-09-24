// Package contracts owns the versioned boundary between clients, services and match pods.
package contracts

import (
	"bytes"
	"crypto/hmac"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"regexp"
	"strconv"
	"strings"
)

var identifier = regexp.MustCompile(`^[a-zA-Z0-9_-]{1,64}$`)
var nonce = regexp.MustCompile(`^[0-9a-f]{32}$`)
var weapon = regexp.MustCompile(`^weapons/[a-zA-Z0-9_-]+(/[a-zA-Z0-9_-]+)*\.asweapon$`)

// Decode bounds input and rejects unknown fields and trailing values.
func Decode(data []byte, out any) error {
	if len(data) > 65536 {
		return errors.New("JSON exceeds 64 KiB")
	}
	d := json.NewDecoder(bytes.NewReader(data))
	d.DisallowUnknownFields()
	if err := d.Decode(out); err != nil {
		return err
	}
	if d.Decode(new(any)) != io.EOF {
		return errors.New("trailing JSON data")
	}
	return nil
}

func PlayerID(id string) bool {
	n, err := strconv.ParseUint(id, 10, 64)
	return err == nil && n != 0 && strconv.FormatUint(n, 10) == id
}

type Rules struct {
	FragLimit int `json:"frag_limit"`
	TimeLimit int `json:"time_limit"`
}
type MatchSpec struct {
	Version         int      `json:"version"`
	ID              string   `json:"id"`
	Map             string   `json:"map"`
	Mode            int      `json:"mode"`
	Slots           int      `json:"slots"`
	Rules           Rules    `json:"rules"`
	ExpectedPlayers []string `json:"expected_players"`
}

func (s MatchSpec) Validate() error {
	if s.Version != 1 || !identifier.MatchString(s.ID) || !identifier.MatchString(s.Map) || len(s.Map) > 48 ||
		s.Mode < 0 || s.Mode > 4 || s.Slots < 1 || s.Slots > 64 || len(s.ExpectedPlayers) < 1 || len(s.ExpectedPlayers) > s.Slots ||
		s.Rules.FragLimit < 0 || s.Rules.FragLimit > 10000 || s.Rules.TimeLimit < 0 || s.Rules.TimeLimit > 1440 ||
		s.Rules.FragLimit+s.Rules.TimeLimit == 0 {
		return errors.New("invalid match specification")
	}
	seen := make(map[string]bool, len(s.ExpectedPlayers))
	for _, id := range s.ExpectedPlayers {
		if !PlayerID(id) || seen[id] {
			return errors.New("invalid or duplicate expected player")
		}
		seen[id] = true
	}
	return nil
}
func DecodeMatchSpec(data []byte) (MatchSpec, error) {
	// Zero is valid for mode and either individual limit, but absence/null is not.
	var wire struct {
		MatchSpec
		Mode  *int `json:"mode"`
		Rules *struct {
			FragLimit *int `json:"frag_limit"`
			TimeLimit *int `json:"time_limit"`
		} `json:"rules"`
	}
	if err := Decode(data, &wire); err != nil {
		return MatchSpec{}, err
	}
	if wire.Mode == nil || wire.Rules == nil || wire.Rules.FragLimit == nil || wire.Rules.TimeLimit == nil {
		return MatchSpec{}, errors.New("missing match mode or rules")
	}
	s := wire.MatchSpec
	s.Mode = *wire.Mode
	s.Rules = Rules{*wire.Rules.FragLimit, *wire.Rules.TimeLimit}
	return s, s.Validate()
}

type Loadout struct {
	Version   int    `json:"version"`
	Primary   string `json:"primary"`
	Secondary string `json:"secondary"`
}

func (l Loadout) Validate(catalog map[string]bool) error {
	if l.Version != 1 {
		return errors.New("unsupported loadout version")
	}
	for _, name := range []string{l.Primary, l.Secondary} {
		if len(name) > 127 || !weapon.MatchString(name) || !catalog[name] {
			return errors.New("unknown weapon asset")
		}
	}
	return nil
}
func DecodeLoadout(data []byte, catalog map[string]bool) (Loadout, error) {
	var l Loadout
	if err := Decode(data, &l); err != nil {
		return l, err
	}
	return l, l.Validate(catalog)
}

type JoinTicket struct {
	Version             int
	PlayerID, MatchID   string
	IssuedAt, ExpiresAt int64
	Nonce               string
}

func (t JoinTicket) valid() bool {
	return t.Version == 1 && PlayerID(t.PlayerID) && identifier.MatchString(t.MatchID) && nonce.MatchString(t.Nonce) &&
		t.IssuedAt > 0 && t.IssuedAt <= 999999999999 && t.ExpiresAt > t.IssuedAt && t.ExpiresAt <= 999999999999 && t.ExpiresAt-t.IssuedAt <= 120
}

// SignJoin uses a per-match 256-bit key. The server must also enforce one-use
// nonces and expected-player membership; a valid signature alone is insufficient.
func SignJoin(key []byte, t JoinTicket) (string, error) {
	if len(key) != 32 || !t.valid() {
		return "", errors.New("invalid join ticket or key")
	}
	payload := fmt.Sprintf("1.%s.%s.%d.%d.%s", t.PlayerID, t.MatchID, t.IssuedAt, t.ExpiresAt, t.Nonce)
	mac := hmac.New(sha256.New, key)
	mac.Write([]byte("aftershock/join/v1\n"))
	mac.Write([]byte(payload))
	return payload + "." + hex.EncodeToString(mac.Sum(nil)), nil
}
func VerifyJoin(key []byte, token, match string, now int64) (JoinTicket, error) {
	fail := errors.New("invalid join ticket")
	if len(token) > 256 || len(key) != 32 {
		return JoinTicket{}, fail
	}
	fields := strings.Split(token, ".")
	if len(fields) != 7 || fields[0] != "1" {
		return JoinTicket{}, fail
	}
	issued, e1 := strconv.ParseInt(fields[3], 10, 64)
	expires, e2 := strconv.ParseInt(fields[4], 10, 64)
	t := JoinTicket{1, fields[1], fields[2], issued, expires, fields[5]}
	if e1 != nil || e2 != nil || !t.valid() || t.MatchID != match || now < issued || now >= expires {
		return JoinTicket{}, fail
	}
	expected, err := SignJoin(key, t)
	// Comparing the canonical complete token also rejects alternate integer/hex encodings.
	if err != nil || !hmac.Equal([]byte(expected), []byte(token)) {
		return JoinTicket{}, fail
	}
	return t, nil
}
