#!/usr/bin/env python3
"""Check seeded local play and input through the real snapshot/usercmd path."""
import argparse
from pathlib import Path
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
        assert before['player']['health'] > 0 and before['camera'], before
        engine.request('input', forward=1, right=0, up=0, yaw=90, pitch=0, fire=False)
        engine.step(30)
        moving = engine.request('state')
        assert before['player']['origin'] != moving['player']['origin']
        engine.request('input', forward=0, right=0, up=0, yaw=90, pitch=0, fire=False)
        engine.step(30)
        stopped = engine.request('state')
        trajectories.append([row['player'] for row in (before, moving, stopped)])
assert trajectories[0] == trajectories[1], trajectories
print('PASS: two seeded map runs produce identical player snapshots through injected usercmds')
