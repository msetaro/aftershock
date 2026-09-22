#!/usr/bin/env python3
"""Validate versioned backend contracts before any service or engine integration."""
import argparse
import hashlib
import hmac
import json
from pathlib import Path
import shlex
import subprocess

import jsonschema
from run import ROOT, SCRATCH, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='gcc')
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-backend')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)

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
sha = args.output/'sha256.o'
run([*shlex.split(args.cc), '-std=c99', '-O2', '-c', 'third_party/sha256/sha-256.c', '-o', sha])
probe = args.output/'join-ticket'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/join_ticket.cpp', 'engine/qcommon/join.cpp', sha, '-o', probe])
payload = '1.18446744073709551615.match-1.2000.2120.'+'ab'*16
signature = hmac.new(bytes([0x42])*32, ('aftershock/join/v1\n'+payload).encode(), hashlib.sha256).hexdigest()
run([probe, payload+'.'+signature])
print('PASS: versioned schemas, Go/native join-ticket parity and one-use nonce retention')
