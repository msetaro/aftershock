#!/usr/bin/env python3
"""Specify cooking a navmesh from owned compiled collision geometry."""
import argparse
import hashlib
import json
import os
import re
import shlex
from pathlib import Path
import subprocess
import sys
from cook import cook
from run import ROOT, SCRATCH, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='gcc')
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-navigation')
args = parser.parse_args()
args.output = args.output.resolve()
os.environ['CXX'] = args.cxx
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
    links=[dict(id=1, start=[-160,-160,0], end=[160,160,0], radius=24,
                bidirectional=True, kind='jump')])
(source/'navigation.json').write_text(json.dumps(definition))
project = source/'navigation-assets.json'
project.write_text(json.dumps(dict(version=1, assets=[
    dict(name='navigation/two_lane', kind='navigation', source='navigation.json')])))
cooked = args.output/'cooked'
first = cook(project, cooked)
assert first['built'] + first['skipped'] == ['navigation/two_lane']
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

sha = args.output/'sha256.o'
run([*shlex.split(args.cc), '-std=c99', '-O2', '-c',
     'third_party/sha256/sha-256.c', '-o', sha])
probe = args.output/'native-probe'
# Explicit source lists remain owned by the same CMake file as the engine.
lists = (ROOT/'cmake/Sources.cmake').read_text()
sources = []
for name in ('DETOUR_SOURCES', 'DETOUR_CROWD_SOURCES'):
    sources += re.search(r'set\('+name+r'\s+(.*?)\)', lists, re.S).group(1).split()
flags = ['-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
         '-ffp-contract=off', '-fno-fast-math', '-fsanitize=undefined',
         '-fno-sanitize-recover=all', '-Ithird_party/recast/Detour/Include',
         '-Ithird_party/recast/DetourCrowd/Include']
objects = []
# Vendored sources keep their own warning policy; owned code stays strict.
for source_path in sources:
    obj = args.output/(Path(source_path).stem+'.o')
    run([*shlex.split(args.cxx), *flags, '-c', source_path, '-o', obj])
    objects.append(obj)
run([*shlex.split(args.cxx), *flags, '-Wall', '-Wextra', '-Werror',
     'tests/probes/navigation.cpp', 'engine/navigation/navigation.cpp', *objects, sha,
     '-o', probe])
run([probe, asset])

# Hierarchical state transitions are authored data. A combat parent's lost-target
# transition applies to both attack and cover children; leaf transitions win.
behavior = dict(version=1, name='guard', initial='patrol', states=[
    dict(name='patrol', action='patrol', transitions=[dict(to='attack', field='visible', op='eq', value=1, min_ms=0),
                                                  dict(to='investigate', field='heard', op='eq', value=1, min_ms=0)]),
    dict(name='combat', action='idle', transitions=[dict(to='investigate', field='visible', op='eq', value=0, min_ms=0)]),
    dict(name='attack', parent='combat', action='attack', transitions=[dict(to='cover', field='health', op='lt', value=0.4, min_ms=500)]),
    dict(name='cover', parent='combat', action='cover', transitions=[dict(to='attack', field='covered', op='eq', value=1, min_ms=1000)]),
    dict(name='investigate', action='investigate', transitions=[dict(to='attack', field='visible', op='eq', value=1, min_ms=0),
                                                            dict(to='patrol', field='time_ms', op='ge', value=5000, min_ms=0)])])
(source/'guard.json').write_text(json.dumps(behavior))
project.write_text(json.dumps(dict(version=1, assets=[
    dict(name='navigation/two_lane', kind='navigation', source='navigation.json'),
    dict(name='behaviors/guard', kind='behavior', source='guard.json')])))
result = cook(project, cooked)
assert result['built'] == ['behaviors/guard'] and result['skipped'] == ['navigation/two_lane']
asset = cooked/'behaviors/guard.asai'
original = hashlib.sha256(asset.read_bytes()).hexdigest()
assert sorted(cook(project, cooked)['skipped']) == ['behaviors/guard', 'navigation/two_lane']
behavior['states'][2]['transitions'][0]['value'] = 0.3
(source/'guard.json').write_text(json.dumps(behavior))
assert cook(project, cooked)['built'] == ['behaviors/guard']
assert hashlib.sha256(asset.read_bytes()).hexdigest() != original
print('PASS: authored hierarchical behavior and isolated incremental source edit')
behavior_probe = args.output/'behavior-probe'
run([*shlex.split(args.cxx), *flags, '-Wall', '-Wextra', '-Werror',
     'tests/probes/behavior.cpp', 'engine/navigation/behavior.cpp', sha, '-o', behavior_probe])
run([behavior_probe, asset])
