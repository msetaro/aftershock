#!/usr/bin/env python3
"""Run cooked navigation and behavior through native bot commands and Pmove."""
import argparse
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from cook import cook
from run import ROOT, SCRATCH
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-navigation-runtime')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-navigation-runtime-', dir=SCRATCH) as temporary:
    root = Path(temporary)
    source = root/'source'
    source.mkdir()
    home = root/'home'
    base = home/('baseoa' if args.content == 'openarena' else 'baseq3')
    with (args.output/'level.log').open('w') as log:
        subprocess.run([sys.executable, 'tools/level', 'tests/assets/levels/two_lane.json', '--output', str(source)],
                       cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    shutil.copytree(source, base)
    navigation = dict(version=1, collision='maps/two_lane.bsp', agent=dict(radius=15,height=56,climb=18,slope=46),
                      cell_size=4, cell_height=2, links=[])
    behavior = dict(version=1, name='patrol', initial='patrol', states=[dict(name='patrol', action='patrol', transitions=[])])
    (source/'navigation.json').write_text(json.dumps(navigation))
    (source/'behavior.json').write_text(json.dumps(behavior))
    (source/'assets.json').write_text(json.dumps(dict(version=1, assets=[
        dict(name='navigation/two_lane', kind='navigation', source='navigation.json'),
        dict(name='behaviors/patrol', kind='behavior', source='behavior.json')])))
    cook(source/'assets.json', base)
    cook(ROOT/'tests/assets/weapons/assets.json', base)
    cook(ROOT/'tests/assets/range.json', base)
    rows = []
    with Engine(args.binary, args.data, args.content, home=home, arguments=[
            '+set', 'g_navigation', 'navigation/two_lane.asnav', '+set', 'g_behavior', 'behaviors/patrol.asai',
            '+set', 'g_weapons', 'weapons/range_rifle.asweapon']) as engine:
        try:
            engine.request('session', dt=20, seed=21)
            engine.request('map', name='two_lane')
            engine.step(50)
            engine.request('exec', command='team spectator')
            engine.step(5)
            engine.request('exec', command='addbot Sarge 3')
            engine.step(100)
            actor = engine.request('actor', owner=1)
            assert actor.get('ai'), 'cooked behavior/navigation did not activate on the native bot'
            assert actor['ai']['state'] == 'patrol' and actor['weapons'][0], actor
            for _ in range(30):
                engine.step(10)
                actor = engine.request('actor', owner=1)
                assert actor['ai']['state'] == 'patrol', actor
                rows.append(actor['ai'])
            assert max(math.dist(rows[0]['position'], row['position']) for row in rows) > 128, 'native bot did not follow its route'
            assert any(row['pathCount'] > 1 and row['complete'] for row in rows), 'no complete navmesh path'
            engine.request('panel', name='AI')
            engine.step(2)
            capture = engine.request('capture', name='navigation-inspector')
            engine.step(2)
            from PIL import Image
            Image.open(base/capture['path']).save(args.output/'navigation-inspector.png')
        finally:
            shutil.copyfile(engine.log_path, args.output/'client.log')
            (args.output/'actors.json').write_text(json.dumps(rows, indent=2))
print('PASS: cooked navmesh/behavior, native bot Pmove, data weapon and AI inspector')
