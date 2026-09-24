#!/usr/bin/env python3
"""Validate versioned ingest envelopes and their Go transport contracts."""
import base64
import copy
import json
import subprocess
import jsonschema
from run import ROOT

schema = json.loads((ROOT/'tools/match/contracts/v1.schema.json').read_text())
jsonschema.Draft202012Validator.check_schema(schema)
validator = jsonschema.Draft202012Validator(schema)
raw = b'  0:00 InitGame: \\mapname\\two_lane'
event = dict(version=1, match_id='match-1', sequence=0, timestamp=0,
             type='InitGame', payload=base64.b64encode(raw).decode())
state = dict(seconds=0, players=None, joins=0, kills=0, scores=None, completed=False)
checkpoint = dict(version=1, match_id='match-1', sequence=len(raw)+1, state=state)
batch = dict(version=1, match_id='match-1', sequence=0, end=len(raw)+1,
             events=[event], checkpoint=checkpoint, final=False)
for value in (event, checkpoint, batch):
    validator.validate(value)
    for field in value:
        missing = copy.deepcopy(value)
        del missing[field]
        assert not validator.is_valid(missing), ('missing required field', field)
    assert not validator.is_valid(dict(value, version=2))
    assert not validator.is_valid(dict(value, unrecognized=True))
for invalid in (dict(event, sequence=-1), dict(event, sequence=2**33+1),
                dict(event, payload='!'), dict(event, type='../type'),
                dict(event, timestamp=-1), dict(batch, events=[event]*65),
                dict(checkpoint, state=dict(state, players={'64': dict(kills=0, deaths=0)})),
                dict(checkpoint, state=dict(state, accounts={'0': dict(score=0, kills=0, deaths=0)}))):
    assert not validator.is_valid(invalid), invalid
subprocess.run(['go', 'test', '-race', './contracts'], cwd=ROOT/'tools/match', check=True)
print('PASS: versioned MatchEvent, MatchCheckpoint and SubmitBatch contracts')
