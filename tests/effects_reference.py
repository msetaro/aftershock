#!/usr/bin/env python3
"""Validate and cook the frozen, original reference effect set without regenerating art."""
import argparse
import hashlib
import json
from pathlib import Path

from cook import cook
from run import ROOT,SCRATCH

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-effects-reference')
args=parser.parse_args()
fixture=ROOT/'tests/assets/effects'
provenance=json.loads((fixture/'provenance.json').read_text())
assert provenance['license']=='CC0-1.0'
for name,expected in provenance['files'].items():
    assert hashlib.sha256((fixture/name).read_bytes()).hexdigest()==expected,name
names=('muzzle','impact_metal','impact_stone','smoke','sparks','dust','shell','explosion','tracer')
definitions={name:json.loads((fixture/(name+'.json')).read_text()) for name in names}
assert definitions['shell']['emitters'][0]['kind']=='mesh'
assert definitions['tracer']['emitters'][0]['kind']=='trail'
assert definitions['smoke']['emitters'][0]['soft'] and definitions['smoke']['emitters'][0]['lit']
assert any(e.get('light',{}).get('intensity',0)>0 for e in definitions['muzzle']['emitters'])
assert any(e.get('collision') for e in definitions['sparks']['emitters'])
assert all(sum(e['capacity'] for e in definition['emitters'])<=256 for definition in definitions.values())
cook(fixture/'assets.json',args.output)
for name in names:
    assert (args.output/f'effects/reference/{name}.asfx').is_file()
assert (args.output/'models/reference_shell.iqm').is_file()
assert cook(fixture/'assets.json',args.output)['built']==[]
print('PASS: original reference effects, fixed provenance, required emitter paths and incremental cooking')
