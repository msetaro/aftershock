#!/usr/bin/env python3
"""Cook data-only weapon variants and exercise fixed-tick, seeded native behavior."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile

from cook import cook
from run import ROOT, ENV, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='gcc')
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-weapons'))
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
sha = args.output / 'sha256.o'
run([*shlex.split(args.cc), '-std=c99', '-O2', '-c', 'third_party/sha256/sha-256.c', '-o', sha])
probe = args.output / 'probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-fno-fast-math', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
     '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/weapons.cpp', 'engine/weapons/weapons.cpp', sha, '-o', probe])
fixture = ROOT / 'tests/assets/weapons'
with tempfile.TemporaryDirectory(prefix='aftershock-weapon-source-') as temporary:
    source = Path(temporary)
    definition = json.loads((fixture / 'rifle.weapon.json').read_text())
    project = json.loads((fixture / 'assets.json').read_text())
    (source / 'rifle.weapon.json').write_text(json.dumps(definition))
    second = dict(definition, name='range_rifle_second', damage=55)
    (source / 'second.weapon.json').write_text(json.dumps(second))
    project['assets'].append({'name': 'weapons/second', 'kind': 'weapon', 'source': 'second.weapon.json'})
    recipe = source / 'assets.json'
    recipe.write_text(json.dumps(project))
    cook(recipe, args.output)
    assert cook(recipe, args.output)['built'] == []
    rifle = args.output / 'weapons/range_rifle.asweapon'
    data = rifle.read_bytes()
    magic, version, size, digest = struct.unpack_from('<8sII32s', data)
    assert magic == b'ASWEAP\0\0' and version == 1 and size == len(data) - 48
    assert digest == hashlib.sha256(data[48:]).digest()
    trace = subprocess.check_output([probe, rifle, args.output / 'weapons/second.asweapon'], cwd=ROOT, env=ENV, timeout=30)
    (args.output / 'shots.txt').write_bytes(trace)
    assert len(trace.splitlines()) == 1000
    print('PASS: 1000 seeded shots and weapon lifecycle;', hashlib.sha256(trace).hexdigest())
    definition['damage'] = 42
    (source / 'rifle.weapon.json').write_text(json.dumps(definition))
    recook = cook(recipe, args.output)
    assert recook['built'] == ['weapons/range_rifle'] and recook['skipped'] == ['weapons/second']
print('PASS: another rifle is data only; versioned cooking, dependency hashes and selective recooking')
