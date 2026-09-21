#!/usr/bin/env python3
"""Exercise local entity edit/save/reload while preserving the map's unknown keys."""
import argparse
import os
from pathlib import Path
import re
import shutil
import struct
import tempfile
import sys
import zipfile
from run import ROOT, build, content_maps
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path)
parser.add_argument('--output', type=Path, default=None)
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
args = parser.parse_args()
temporary = tempfile.TemporaryDirectory(prefix='aftershock-entity-edit-', dir=os.environ.get('AFTERSHOCK_SCRATCH'))
args.output = args.output.resolve() if args.output else Path(temporary.name) / 'output'
args.output.mkdir(parents=True, exist_ok=True)
# Entity editing belongs to this repository's native game, also when using OA art.
if not args.binary:
    args.binary = build(args.output / 'build', ['BUILD_SERVER=0', 'AFTERSHOCK_DEVTOOLS=1']) / 'quake3e.x64'
paks = sorted(args.data.resolve().glob('*.pk3'))
map_name = content_maps(args.content)[0]
original = None
for pak in paks:
    with zipfile.ZipFile(pak) as archive:
        if f'maps/{map_name}.bsp' in archive.namelist():
            bsp = archive.read(f'maps/{map_name}.bsp')
            offset, length = struct.unpack_from('<ii', bsp, 8)
            original = bsp[offset:offset + length].rstrip(b'\0')
assert original is not None

def tokens(text):
    # Trusted installed map text; compare quoted tokens, preserving backslashes.
    return re.findall(rb'"[^"]*"|[{}]', text)

with temporary, Engine(args.binary, args.data, args.content, home=Path(temporary.name) / 'home') as engine:
    base = engine.base
    engine.request('session', dt=8, seed=123)
    engine.request('map', name=map_name)
    engine.step(200)
    engine.request('panel', name='World')
    for collision, navigation in ((True, False), (False, True), (True, True)):
        engine.request('world', collision=collision, navigation=navigation, entities=False, radius=2048)
        engine.step(2)
        assert engine.request('editor.state')['world']['lines'] > 0, 'collision/navigation cache was empty'
    engine.request('entity.save')
    entity = engine.request('entity.spawn', classname='target_position', x=1, y=2, z=128)['entity']
    for key, value in (('targetname', 'dev_tools_entity_test'), ('count', '7'), ('origin', '4 5 129'),
                       ('angle', '45'), ('angles', '10 20 30'), ('angle', '90')):
        engine.request('entity.set', entity=entity, key=key, value=value)
    assert engine.request('entity.get', entity=entity, key='count')['value'] == '7'
    engine.request('entity.save')
    engine.request('entity.delete', entity=entity)
    engine.request('entity.save')
    engine.request('cvar.set', name='dev_entityFile', value=f'maps/{map_name}.dev.001.ent')
    engine.request('entity.reload')
    engine.step(200)
    # Map records can acquire new runtime ids on restart; locate by authored key.
    offset = 0
    found = []
    while offset is not None:
        page = engine.request('entity.list', offset=offset)
        for row in page['entities']:
            if row['classname'] == 'target_position' and row['source'] >= 0:
                value = engine.request('entity.get', entity=row['entity'], key='targetname')['value']
                if value == 'dev_tools_entity_test':
                    found.append(row['entity'])
        offset = page['next']
    assert len(found) == 1, found
    entity = found[0]
    assert engine.request('entity.get', entity=entity, key='count')['value'] == '7'
    for key, expected in (('origin', [4, 5, 129]), ('angles', [0, 90, 0])):
        value = engine.request('entity.get', entity=entity, key=key)['value']
        assert list(map(float, value.split())) == expected, (key, value)
    engine.request('entity.save')
    saved = [(base / 'maps' / f'{map_name}.dev.{i:03d}.ent').read_bytes() for i in range(4)]
    assert tokens(saved[0]) == tokens(original), 'original/unknown map keys changed'
    assert tokens(saved[2]) == tokens(original), 'deleting the new entity changed original records'
    assert tokens(saved[1]) == tokens(saved[3]), 'saved entity fields did not survive reload'
    assert b'"targetname" "dev_tools_entity_test"' in saved[1]
    for i in range(4):
        shutil.copyfile(base / 'maps' / f'{map_name}.dev.{i:03d}.ent', args.output / f'entities-{i}.ent')
    shutil.copyfile(engine.log_path, args.output / 'client.log')
print('PASS: spawn/edit/delete/save/reload, angle aliases, numbered saves and preservation of every original map key')
