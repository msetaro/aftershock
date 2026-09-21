#!/usr/bin/env python3
"""Exercise shared panel actions through the local channel, without pixel input."""
import argparse
from pathlib import Path
import sys
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
args = parser.parse_args()
models = set()
for pak in args.data.glob('*.pk3'):
    with zipfile.ZipFile(pak) as archive:
        models.update(name for name in archive.namelist()
                      if name.startswith('models/players/') and name.endswith('/lower.md3'))
assert models, 'installed player model required'
with Engine(args.binary, args.data, args.content) as engine:
    engine.request('session', dt=8, seed=123)
    engine.request('map', name='oa_dm1' if args.content == 'openarena' else 'q3dm17')
    engine.step(200)
    engine.request('panel', name='World')
    engine.request('world', collision=True, navigation=True, entities=True, radius=1024)
    engine.step(3)
    world = engine.request('editor.state')
    assert world['panel'] == 'World' and world['frames'] > 0, world
    assert world['world']['lines'] > 0 and world['world']['collision'] and world['world']['navigation'], world
    engine.request('panel', name='Animation')
    engine.request('animation.load', path=sorted(models)[0])
    engine.step(3)
    animation = engine.request('editor.state')['animation']
    assert animation['model'] > 0 and animation['previews'] > 0, animation
    engine.request('animation.set', field='frame', value=1)
    engine.step(2)
    assert engine.request('editor.state')['animation']['frame'] == 1
    engine.request('animation.set', field='play', value=1)
    engine.step(20)
    assert engine.request('editor.state')['animation']['frame'] != 1
print('PASS: shared world and animation panel actions render and report structured state without pixel clicks')
