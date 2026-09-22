#!/usr/bin/env python3
"""Validate versioned backend contracts before any service or engine integration."""
import json
import subprocess

import jsonschema
from run import ROOT

schema = json.loads((ROOT/'tools/match/contracts/v1.schema.json').read_text())
jsonschema.Draft202012Validator.check_schema(schema)
validator = jsonschema.Draft202012Validator(schema)
cases = [
    dict(version=1, id='match-1', map='two_lane', mode=0, slots=8,
         rules=dict(frag_limit=10, time_limit=10), expected_players=['1','2']),
    dict(version=1, primary='weapons/range_rifle.asweapon', secondary='weapons/sidearm.asweapon'),
    '1.1.match-1.2000.2120.'+'ab'*16+'.'+'00'*32,
]
for case in cases:
    validator.validate(case)
for case in [dict(cases[0], version=2), dict(cases[0], command='quit'),
             dict(cases[0], expected_players=['1','1']), dict(cases[1], primary='../weapon'),
             dict(cases[1], damage=999), cases[2]+'.extra']:
    assert not validator.is_valid(case), case
subprocess.run(['go', 'test', '-race', './contracts'], cwd=ROOT/'tools/match', check=True)
print('PASS: versioned match/loadout schemas and signed, bounded join-ticket contracts')
