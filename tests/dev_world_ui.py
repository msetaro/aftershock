#!/usr/bin/env python3
"""Render entity/world panels and exercise their shared controls without pixel clicks."""
import argparse
import os
from pathlib import Path
import shutil
import sys
import tempfile

from PIL import Image
from run import ROOT, build, content_maps
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path)
parser.add_argument('--output', type=Path)
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='aftershock-world-ui-', dir=os.environ.get('AFTERSHOCK_SCRATCH')) as temporary:
    output = args.output.resolve() if args.output else Path(temporary)
    output.mkdir(parents=True, exist_ok=True)
    binary = args.binary or build(output / 'build', ['BUILD_SERVER=0', 'AFTERSHOCK_DEVTOOLS=1']) / 'quake3e.x64'
    with Engine(binary, args.data, args.content) as engine:
        engine.request('session', dt=8, seed=123)
        engine.request('map', name=content_maps(args.content)[0])
        engine.step(200)
        engine.request('panel', name='Entities')
        position = engine.request('entity.at_camera')['origin']
        entity = engine.request('entity.spawn', classname='target_position', **dict(zip(('x', 'y', 'z'), position)))['entity']
        assert engine.request('entity.pick')['entity'] == entity
        engine.step(3)
        state = engine.request('editor.state')
        assert state['panel'] == 'Entities' and state['entity'] == entity, state
        assert state['labels'] > 0 and state['lines'] >= 12, state
        capture = engine.request('capture', name='entities')
        engine.step(2)
        shutil.copyfile(engine.base / capture['path'], output / 'entities.png')
        engine.request('panel', name='World')
        engine.request('world', collision=True, navigation=True, entities=True, radius=512)
        engine.step(3)
        state = engine.request('editor.state')
        assert state['panel'] == 'World' and state['world']['lines'] > 0, state
        assert state['world']['collision'] and state['world']['navigation'], state
        assert state['lines'] > 12 and state['labels'] > 0, state
        capture = engine.request('capture', name='world')
        engine.step(2)
        shutil.copyfile(engine.base / capture['path'], output / 'world.png')
        shutil.copyfile(engine.log_path, output / 'client.log')
    for name in ('entities', 'world'):
        with Image.open(output / (name + '.png')) as image:
            assert image.size == (640, 480)
print('PASS: shared panel spawn, crosshair picking, collision/navigation wireframes and projected entity label')
