#!/usr/bin/env python3
"""Exercise shared target-range controls and inspect authoritative weapon state."""
import argparse
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile

from cook import cook
from run import ROOT, content_maps
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--output', type=Path)
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='aftershock-range-ui-', dir=os.environ.get('AFTERSHOCK_SCRATCH')) as temporary:
    home = Path(temporary) / 'home'
    output = args.output.resolve() if args.output else Path(temporary) / 'output'
    output.mkdir(parents=True, exist_ok=True)
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    sources = Path(temporary) / 'sources'
    sources.mkdir()
    definition = json.loads((ROOT / 'tests/assets/weapons/rifle.weapon.json').read_text())
    (sources / 'rifle.json').write_text(json.dumps(definition))
    (sources / 'second.json').write_text(json.dumps(dict(definition, name='range_second', damage=55)))
    project = {'version': 1, 'assets': [{'name': 'weapons/' + name, 'kind': 'weapon', 'source': source}
               for name, source in [('range_rifle', 'rifle.json'), ('second', 'second.json')]]}
    (sources / 'assets.json').write_text(json.dumps(project))
    cook(sources / 'assets.json', base)
    cook(ROOT / 'tests/assets/range.json', base)
    arguments = ['+set', 'g_weapons', 'weapons/range_rifle.asweapon weapons/second.asweapon',
                 '+set', 'g_rewind', '1', '+set', 'g_animationBody', 'animations/anim_body.asanim',
                 '+set', 'g_animationRifle', 'animations/range_rifle.asanim']
    with Engine(args.binary, args.data, args.content, home=home, arguments=arguments) as engine:
        engine.request('session', dt=20, seed=123)
        engine.request('map', name=content_maps(args.content)[0])
        engine.step(100)
        engine.request('panel', name='Range')
        engine.request('range', action='inspect', path='weapons/range_rifle.asweapon')
        engine.step(3)
        state = engine.request('editor.state')
        assert state['panel'] == 'Range' and state['range']['loaded'], state
        assert state['range']['name'] == definition['name'], state
        engine.request('range', action='target')
        engine.step(3)
        entities = []
        offset = 0
        while offset is not None:
            page = engine.request('entity.list', offset=offset)
            entities.extend(page['entities'])
            offset = page['next']
        assert any(row['classname'] == 'rewind_target' for row in entities), entities
        engine.request('range', action='select', value=2)
        engine.step(30)
        actor = engine.request('actor')
        assert actor['weapons'][0]['selected'] == 1 and actor['weapons'][0]['name'] == 'range_second', actor
        assert all(rig and rig['state'] for rig in actor['animation']), actor
        before = actor['weapons'][0]['sequence']
        ammo = actor['weapons'][0]['magazine'] + actor['weapons'][0]['chamber']
        engine.request('range', action='fire')
        engine.step(20)
        weapon = engine.request('actor')['weapons'][0]
        assert weapon['sequence'] > before and weapon['magazine'] + weapon['chamber'] < ammo, weapon
        engine.request('range', action='reload')
        engine.step(8)
        assert engine.request('actor')['weapons'][0]['reloadStage'] != 0xffffffff
        engine.step(150)
        weapon = engine.request('actor')['weapons'][0]
        assert weapon['reloadStage'] == 0xffffffff and weapon['magazine'] + weapon['chamber'] >= ammo, weapon
        engine.request('range', action='ads', value=1)
        engine.step(30)
        assert engine.request('actor')['weapons'][0]['adsQ16'] > 0
        engine.request('range', action='ads', value=0)
        capture = engine.request('capture', name='range-panel')
        engine.step(2)
        shutil.copyfile(base / capture['path'], output / 'range-panel.png')
        shutil.copyfile(engine.log_path, output / 'client.log')
print('PASS: shared range panel, moving target, data-only rifle selection, trigger, reload, ADS and animation state')
