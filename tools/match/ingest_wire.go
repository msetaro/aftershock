package main

import (
	"encoding/base64"
	"encoding/json"
	"errors"
	"unicode/utf8"

	"github.com/msetaro/aftershock/tools/match/contracts"
)

// Keep existing ASCII pending/stub records byte-compatible. Only records that
// JSON strings cannot preserve use the optional exact-byte representation.
func (b batch) MarshalJSON() ([]byte, error) {
	type plain batch
	value := struct {
		plain
		RawEvents []string `json:"raw_events,omitempty"`
	}{plain: plain(b)}
	for _, raw := range b.Events {
		if !utf8.ValidString(raw) {
			value.Events = nil
			value.RawEvents = make([]string, len(b.Events))
			for i, raw := range b.Events {
				value.RawEvents[i] = base64.StdEncoding.EncodeToString([]byte(raw))
			}
			break
		}
	}
	return json.Marshal(value)
}
func (b *batch) UnmarshalJSON(data []byte) error {
	type plain batch
	var value struct {
		plain
		RawEvents []string `json:"raw_events,omitempty"`
	}
	if err := strictJSON(data, &value); err != nil {
		return err
	}
	if len(value.RawEvents) != 0 {
		if len(value.Events) != 0 || len(value.RawEvents) > 64 {
			return errors.New("mixed or oversized pending events")
		}
		value.Events = make([]string, len(value.RawEvents))
		for i, encoded := range value.RawEvents {
			if len(encoded) > 5460 {
				return errors.New("oversized pending event")
			}
			raw, err := base64.StdEncoding.DecodeString(encoded)
			if err != nil || base64.StdEncoding.EncodeToString(raw) != encoded {
				return errors.New("invalid pending event encoding")
			}
			value.Events[i] = string(raw)
		}
	}
	*b = batch(value.plain)
	return nil
}

func matchBatch(b batch) (contracts.MatchBatch, error) {
	result := contracts.MatchBatch{Version: 1, MatchID: b.Match, Sequence: b.Start, End: b.End, Final: b.Final,
		Events: []contracts.MatchEvent{}, Checkpoint: contracts.MatchCheckpoint{Version: 1, MatchID: b.Match, Sequence: b.End}}
	data, err := json.Marshal(b.Checkpoint)
	if err != nil {
		return result, err
	}
	if err = json.Unmarshal(data, &result.Checkpoint.State); err != nil {
		return result, err
	}
	offset := b.Start
	for _, raw := range b.Events {
		event, err := contracts.NewMatchEvent(b.Match, offset, raw)
		if err != nil {
			return result, err
		}
		result.Events = append(result.Events, event)
		offset += int64(len(raw) + 1)
	}
	return result, result.Validate()
}

func legacyBatch(wire contracts.MatchBatch) (batch, error) {
	b := batch{Version: 1, Match: wire.MatchID, Start: wire.Sequence, End: wire.End, Final: wire.Final}
	if err := wire.Validate(); err != nil {
		return b, err
	}
	for _, event := range wire.Events {
		raw, _ := event.Raw()
		b.Events = append(b.Events, raw)
	}
	data, err := json.Marshal(wire.Checkpoint.State)
	if err == nil {
		err = json.Unmarshal(data, &b.Checkpoint)
	}
	return b, err
}
