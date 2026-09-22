#!/usr/bin/env python3
"""Specify cooking a navmesh from owned compiled collision geometry."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from cook import cook
from run import ROOT, SCRATCH

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-navigation')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
source = args.output/'source'
source.mkdir(exist_ok=True)
with (args.output/'level.log').open('w') as log:
    subprocess.run([sys.executable, 'tools/level', 'tests/assets/levels/two_lane.json', '--output', str(source)],
                   cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
# The source is actual CM collision, including brushes and patches, not visible
# render triangles. Recast generates walkable polygons offline.
definition = dict(version=1, collision='maps/two_lane.bsp', agent=dict(
    radius=15, height=56, climb=18, slope=46), cell_size=4, cell_height=2,
    links=[dict(id=1, start=[-160,-160,48], end=[160,160,48], radius=24,
                bidirectional=True, kind='jump')])
(source/'navigation.json').write_text(json.dumps(definition))
project = source/'navigation-assets.json'
project.write_text(json.dumps(dict(version=1, assets=[
    dict(name='navigation/two_lane', kind='navigation', source='navigation.json')])))
cooked = args.output/'cooked'
first = cook(project, cooked)
assert first['built'] == ['navigation/two_lane']
asset = cooked/'navigation/two_lane.asnav'
assert asset.is_file() and asset.stat().st_size > 128
manifest = json.loads((cooked/'navigation/two_lane.manifest.json').read_text())
assert {entry['path'] for entry in manifest['inputs']} == {'navigation.json', 'maps/two_lane.bsp'}
original = hashlib.sha256(asset.read_bytes()).hexdigest()
assert cook(project, cooked)['skipped'] == ['navigation/two_lane']
asset.unlink()
assert cook(project, cooked)['built'] == ['navigation/two_lane']
assert hashlib.sha256(asset.read_bytes()).hexdigest() == original
print('PASS: collision BSP navigation asset, transitive hashes and repeated deterministic cook')
