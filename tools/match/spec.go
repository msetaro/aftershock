package main

import (
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"path/filepath"
	"regexp"
	"strconv"

	"github.com/msetaro/aftershock/tools/match/contracts"
)

type spec struct {
	ID              string   `json:"id"`
	Map             string   `json:"map"`
	Mode            int      `json:"mode"`
	FragLimit       int      `json:"frag_limit"`
	TimeLimit       int      `json:"time_limit"`
	Players         int      `json:"players"`
	Password        string   `json:"password"`
	Token           string   `json:"token"`
	JoinKey         string   `json:"join_key,omitempty"`
	ExpectedPlayers []string `json:"expected_players,omitempty"`
}

var identifier = regexp.MustCompile(`^[a-zA-Z0-9_-]{1,64}$`)
var secret = regexp.MustCompile(`^[a-zA-Z0-9_.-]{8,128}$`)

func strictJSON(data []byte, out any) error {
	return contracts.Decode(data, out)
}

func decodeSpec(data []byte) (spec, error) {
	// Backend allocations carry the public v1 contract plus private pod credentials.
	var allocation struct {
		Match   json.RawMessage `json:"match"`
		JoinKey string          `json:"join_key"`
		Token   string          `json:"token"`
	}
	if err := strictJSON(data, &allocation); err == nil && len(allocation.Match) != 0 {
		m, err := contracts.DecodeMatchSpec(allocation.Match)
		if err != nil {
			return spec{}, err
		}
		s := spec{ID: m.ID, Map: m.Map, Mode: m.Mode, Players: m.Slots,
			FragLimit: m.Rules.FragLimit, TimeLimit: m.Rules.TimeLimit,
			JoinKey: allocation.JoinKey, ExpectedPlayers: m.ExpectedPlayers, Token: allocation.Token}
		if s.JoinKey == "" {
			return spec{}, errors.New("allocation requires a join key")
		}
		return s, s.validate()
	}
	// Retain the original #28 password format and the private persisted pod format.
	var s spec
	if err := strictJSON(data, &s); err != nil {
		return s, err
	}
	return s, s.validate()
}
func writeJoinConfig(home string, s spec) error {
	if err := s.validate(); err != nil {
		return err
	}
	if s.JoinKey == "" {
		return nil
	}
	return writeJSON(filepath.Join(home, "match-join.json"), struct {
		Version int      `json:"version"`
		Match   string   `json:"match_id"`
		Key     string   `json:"join_key"`
		Players []string `json:"expected_players"`
	}{1, s.ID, s.JoinKey, s.ExpectedPlayers})
}
func (s spec) validate() error {
	if s.JoinKey != "" {
		key, err := hex.DecodeString(s.JoinKey)
		if err != nil || len(key) != 32 || hex.EncodeToString(key) != s.JoinKey || s.Password != "" || !secret.MatchString(s.Token) {
			return errors.New("invalid private allocation credentials")
		}
		return (contracts.MatchSpec{Version: 1, ID: s.ID, Map: s.Map, Mode: s.Mode, Slots: s.Players,
			Rules: contracts.Rules{FragLimit: s.FragLimit, TimeLimit: s.TimeLimit}, ExpectedPlayers: s.ExpectedPlayers}).Validate()
	}
	if len(s.ExpectedPlayers) != 0 {
		return errors.New("expected players require signed admission")
	}
	if !identifier.MatchString(s.ID) || !identifier.MatchString(s.Map) || len(s.Map) > 48 || !secret.MatchString(s.Password) || !secret.MatchString(s.Token) ||
		s.Mode < 0 || s.Mode > 4 || s.Players < 1 || s.Players > 64 || s.FragLimit < 0 || s.FragLimit > 10000 || s.TimeLimit < 0 || s.TimeLimit > 1440 || s.FragLimit+s.TimeLimit == 0 {
		return errors.New("invalid match identifier, password/token, mode, limits or player count")
	}
	return nil
}
func serverArgs(s spec, content, home, game string, port int, warm bool) ([]string, error) {
	if err := s.validate(); err != nil {
		return nil, err
	}
	if !identifier.MatchString(game) || port < 1024 || port > 65535 {
		return nil, errors.New("invalid game directory or unprivileged port")
	}
	args := []string{}
	set := func(k, v string) { args = append(args, "+set", k, v) }
	set("fs_basepath", content)
	set("fs_homepath", home)
	set("fs_basegame", game)
	set("dedicated", "1")
	set("net_enabled", "1")
	set("net_port", strconv.Itoa(port))
	set("sv_hostname", "Aftershock match")
	set("sv_pure", "1")
	set("sv_allowDownload", "0")
	set("bot_enable", "0")
	set("g_log", "games.log")
	set("g_logSync", "1")
	set("sv_maxclients", strconv.Itoa(s.Players))
	set("g_gametype", strconv.Itoa(s.Mode))
	set("g_password", s.Password)
	set("sv_exitOnMatchEnd", "1")
	set("fraglimit", strconv.Itoa(s.FragLimit))
	set("timelimit", strconv.Itoa(s.TimeLimit))
	if warm {
		set("fraglimit", "0")
		set("timelimit", "0")
	}
	set("capturelimit", "0")
	if s.JoinKey != "" {
		args = append(args, "+joinconfig")
	}
	return append(args, "+map", s.Map), nil
}

type playerStats struct {
	Kills  int `json:"kills"`
	Deaths int `json:"deaths"`
}
type checkpoint struct {
	Seconds   int                    `json:"seconds"`
	Players   map[string]playerStats `json:"players"`
	Joins     int                    `json:"joins"`
	Kills     int                    `json:"kills"`
	Scores    map[string]int         `json:"scores"`
	Completed bool                   `json:"completed"`
}

func (c *checkpoint) add(line string) {
	var minutes, seconds int
	var event string
	if _, err := fmt.Sscanf(line, "%d:%d %s", &minutes, &seconds, &event); err != nil {
		return
	}
	c.Seconds = minutes*60 + seconds
	switch event {
	case "ClientBegin:":
		c.Joins++
	case "Kill:":
		c.Kills++
		var attacker, victim, weapon int
		if n, _ := fmt.Sscanf(line, "%d:%d Kill: %d %d %d:", &minutes, &seconds, &attacker, &victim, &weapon); n == 5 {
			if c.Players == nil {
				c.Players = map[string]playerStats{}
			}
			if attacker >= 0 && attacker < 64 && attacker != victim {
				key := strconv.Itoa(attacker)
				p := c.Players[key]
				p.Kills++
				c.Players[key] = p
			}
			if victim >= 0 && victim < 64 {
				key := strconv.Itoa(victim)
				p := c.Players[key]
				p.Deaths++
				c.Players[key] = p
			}
		}
	case "Exit:":
		c.Completed = true
	case "score:":
		var score, ping, client int
		if n, _ := fmt.Sscanf(line, "%d:%d score: %d ping: %d client: %d", &minutes, &seconds, &score, &ping, &client); n == 5 {
			if c.Scores == nil {
				c.Scores = map[string]int{}
			}
			c.Scores[strconv.Itoa(client)] = score
		}
	}
}
