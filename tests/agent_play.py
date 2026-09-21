#!/usr/bin/env python3
"""Check seeded local play and input through the real snapshot/usercmd path."""
import argparse
from pathlib import Path
from PIL import Image
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
args = parser.parse_args()
map_name = 'oa_dm1' if args.content == 'openarena' else 'q3dm17'
trajectories = []
for _ in range(2):
    with Engine(args.binary, args.data, args.content) as engine:
        engine.request('hello')
        engine.request('session', dt=8, seed=123)
        engine.request('map', name=map_name)
        engine.step(200)
        before = engine.request('state')
        assert before['player'] and before['player']['health'] > 0 and before['camera'], (before, engine.log_path.read_text(errors='replace')[-5000:])
        engine.request('input', forward=1, right=0, up=0, yaw=90, pitch=0, fire=False)
        engine.step(30)
        moving = engine.request('state')
        assert before['player']['origin'] != moving['player']['origin']
        engine.request('input', forward=0, right=0, up=0, yaw=90, pitch=0, fire=False)
        engine.step(30)
        stopped = engine.request('state')
        trajectories.append([row['player'] for row in (before, moving, stopped)])
        entity = engine.request('entity.spawn', classname='target_position', x=1, y=2, z=128)['entity']
        engine.request('entity.set', entity=entity, key='targetname', value='agent-check')
        assert engine.request('entity.get', entity=entity, key='targetname')['value'] == 'agent-check'
        listed = engine.request('entity.list', offset=entity, limit=1)['entities']
        assert listed[0]['entity'] == entity and listed[0]['origin'] == [1, 2, 128]
        engine.request('entity.delete', entity=entity)
        assert all(row['entity'] != entity for row in engine.request('entity.list', offset=entity, limit=1)['entities'])
        capture = engine.request('capture', name='agent-check')
        engine.step(2)
        with Image.open(engine.base/capture['path']) as image:
            assert image.format == 'PNG' and image.size == (640, 480)
            assert len(image.convert('RGB').getcolors(640*480)) > 100
        profile = engine.request('profile')
        assert profile['samples'] == 262 and 0 <= profile['p50_ms'] <= profile['p95_ms'] <= profile['p99_ms']
        assert profile['cpu'] and 'snapshots' in profile['network']
assert trajectories[0] == trajectories[1], trajectories
print('PASS: two seeded map runs produce identical player snapshots through injected usercmds')
