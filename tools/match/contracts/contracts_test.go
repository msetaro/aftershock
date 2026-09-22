package contracts

import (
	"bytes"
	"strings"
	"testing"
)

func TestJoinTicket(t *testing.T) {
	key := bytes.Repeat([]byte{0x42}, 32)
	claims := JoinTicket{Version: 1, PlayerID: "18446744073709551615", MatchID: "match-1", IssuedAt: 2000, ExpiresAt: 2120, Nonce: strings.Repeat("ab", 16)}
	token, err := SignJoin(key, claims)
	if err != nil {
		t.Fatal(err)
	}
	if token != "1.18446744073709551615.match-1.2000.2120.abababababababababababababababab.aeb7bddd7a12dc3c3b4ecd52679ecd620a81ffdbc4f1547146baabcf898013e3" {
		t.Fatal("ticket differs from independent Python HMAC oracle")
	}
	got, err := VerifyJoin(key, token, "match-1", 2050)
	if err != nil || got != claims {
		t.Fatalf("round trip: %+v %v", got, err)
	}
	for _, bad := range []string{token + ".extra", "2" + token[1:], token[:len(token)-1], strings.Replace(token, claims.PlayerID, "1", 1), strings.Replace(token, ".match-1.", ".match-2.", 1), strings.Replace(token, ".2000.", ".02000.", 1)} {
		if _, err := VerifyJoin(key, bad, "match-1", 2050); err == nil {
			t.Fatal("malformed/tampered ticket accepted")
		}
	}
	for _, now := range []int64{1999, 2120, 2200} {
		if _, err := VerifyJoin(key, token, "match-1", now); err == nil {
			t.Fatal("invalid ticket time accepted", now)
		}
	}
	if _, err := VerifyJoin(key, token, "other-match", 2050); err == nil {
		t.Fatal("wrong match accepted")
	}
	if _, err := VerifyJoin(bytes.Repeat([]byte{0x43}, 32), token, "match-1", 2050); err == nil {
		t.Fatal("wrong key accepted")
	}
	if _, err := SignJoin(key[:16], claims); err == nil {
		t.Fatal("short signing key accepted")
	}
	for _, id := range []string{"0", "01", "18446744073709551616", "-1", "1;quit"} {
		bad := claims
		bad.PlayerID = id
		if _, err := SignJoin(key, bad); err == nil {
			t.Fatal("invalid player identity accepted", id)
		}
	}
	bad := claims
	bad.ExpiresAt++
	if _, err := SignJoin(key, bad); err == nil {
		t.Fatal("ticket over 120 seconds accepted")
	}
	bad = claims
	bad.Version = 2
	if _, err := SignJoin(key, bad); err == nil {
		t.Fatal("unknown version accepted")
	}
}

func TestMatchAndLoadout(t *testing.T) {
	data := []byte(`{"version":1,"id":"match-1","map":"two_lane","mode":0,"slots":8,"rules":{"frag_limit":10,"time_limit":10},"expected_players":["1","2"]}`)
	match, err := DecodeMatchSpec(data)
	if err != nil || match.ID != "match-1" || len(match.ExpectedPlayers) != 2 {
		t.Fatal(match, err)
	}
	for _, bad := range []string{strings.Replace(string(data), `"mode":0,`, ``, 1), strings.Replace(string(data), `"mode":0`, `"mode":null`, 1), strings.Replace(string(data), `"frag_limit":10,`, ``, 1), strings.Replace(string(data), `"version":1`, `"version":2`, 1), strings.Replace(string(data), `"slots":8`, `"slots":1`, 1), strings.Replace(string(data), `["1","2"]`, `["1","1"]`, 1), strings.Replace(string(data), `"two_lane"`, `"two_lane;quit"`, 1), strings.Replace(string(data), `"mode":0`, `"mode":0,"command":"quit"`, 1), string(data) + `{}`} {
		if _, err := DecodeMatchSpec([]byte(bad)); err == nil {
			t.Fatal("invalid match accepted", bad)
		}
	}
	weapons := map[string]bool{"weapons/range_rifle.asweapon": true, "weapons/sidearm.asweapon": true}
	loadout := []byte(`{"version":1,"primary":"weapons/range_rifle.asweapon","secondary":"weapons/sidearm.asweapon"}`)
	if _, err := DecodeLoadout(loadout, weapons); err != nil {
		t.Fatal(err)
	}
	for _, bad := range []string{strings.Replace(string(loadout), `"version":1`, `"version":2`, 1), strings.Replace(string(loadout), "range_rifle", "unknown", 1), strings.Replace(string(loadout), "weapons/sidearm", "../sidearm", 1), strings.Replace(string(loadout), `"secondary":`, `"damage":999,"secondary":`, 1)} {
		if _, err := DecodeLoadout([]byte(bad), weapons); err == nil {
			t.Fatal("invalid loadout accepted", bad)
		}
	}
}
