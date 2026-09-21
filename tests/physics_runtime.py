#!/usr/bin/env python3
"""Exercise cosmetic prop contacts, bounded storage and map teardown in the client."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

from run import ROOT, SCRATCH, content_maps, content_settings

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', required=True, type=Path)
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-physics-runtime')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
assert paks and args.binary.is_file(), 'installed content and an existing client are required'
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
assert len(icds) == 1, 'exactly one lavapipe ICD is required'
env = dict(os.environ, VK_ICD_FILENAMES=str(icds[0]), VK_DRIVER_FILES=str(icds[0]),
           LIBGL_ALWAYS_SOFTWARE='1', LP_NUM_THREADS='1')
with tempfile.TemporaryDirectory(prefix='aftershock-physics-') as temporary:
    home = Path(temporary)
    base = home/('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    for pak in paks:
        (base/pak.name).symlink_to(pak)
    commands = []
    for map_name in content_maps(args.content):
        commands += [f'map {map_name}', 'wait 120', 'physics_status',
                     'physics_prop grenade drop', 'physics_prop box', 'physics_status']
        for _ in range(30):
            commands += ['wait 6', 'physics_status']
    commands += ['disconnect', 'quit']
    (base/'physics-test.cfg').write_text('\n'.join(commands)+'\n')
    command = ['xvfb-run', '-a', str(args.binary.resolve()), '+set', 'fs_basepath', str(home),
               '+set', 'fs_homepath', str(home), *content_settings(args.content),
               '+set', 'r_fullscreen', '0', '+set', 'r_mode', '3', '+set', 's_initsound', '0',
               '+set', 'sv_pure', '0', '+set', 'net_ip', '127.0.0.1', '+set', 'com_maxfps', '125',
               '+set', 'com_maxfpsUnfocused', '125', '+set', 'fixedtime', '16',
               '+set', 'cl_autoRecordDemo', '0', '+exec', 'physics-test.cfg']
    with (args.output/'client.log').open('wb') as log:
        subprocess.run(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT,
                       timeout=120, check=True)
text = (args.output/'client.log').read_text(errors='replace')
worlds = text.split('Physics world: ')[1:]
assert len(worlds) == len(content_maps(args.content)), 'every map must initialize physics'
for world in worlds:
    stats = re.findall(r'Physics status: steps=(\d+) arena=(\d+) allocations=(\d+) live=(\d+)', world)
    assert len(stats) == 32, 'all scheduled samples must execute'
    assert len({row[1:] for row in stats}) == 1, 'frame/spawn/query storage must stay fixed'
    assert int(stats[-1][0]) > int(stats[0][0]), 'fixed physics steps must execute'
    bodies = re.findall(r'Physics body: slot=(\d+) kind=(\w+) position=([^ ]+) velocity=([^\r\n]+)', world)
    grenades = [tuple(map(float, row[3].split(','))) for row in bodies if row[1] == 'grenade']
    assert any(v[2] < -.5 for v in grenades), 'grenade must fall under Jolt gravity'
    assert any(v[2] > .5 for v in grenades), 'grenade must bounce from map geometry'
    assert len({row[2] for row in bodies if row[1] == 'box'}) > 1, 'prop must react'
assert re.findall(r'Physics shutdown: live=(\d+)', text) == ['0'] * len(worlds), 'each map must release dependency blocks'
assert 'arena exhausted' not in text and 'contact capacity exceeded' not in text
print('PASS: cosmetic prop motion, grenade bounce, fixed storage and two map lifetimes')

# A replicated ordinary death must activate the cooked skeleton, not just a
# directly spawned test body. Fall damage preserves the corpse (suicide gibs it).
import shutil
import sys
from cook import cook
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

with Engine(args.binary, args.data, args.content, arguments=['+set', 'handicap', '1']) as engine:
    try:
        cook(ROOT/'tests/assets/animation/rigs.json', engine.base)
        engine.request('session', dt=8, seed=123)
        for name, value in [('g_animationBody', 'animations/anim_body.asanim'),
                            ('g_animationRifle', 'animations/anim_rifle.asanim'),
                            ('cg_thirdPerson', '1')]:
            engine.request('exec', command=f'set {name} {value}')
        engine.request('map', name='oa_dm7' if args.content == 'openarena' else 'q3dm1')
        engine.step(200)
        origin = engine.request('state')['player']['origin']
        engine.request('exec', command='physics_status')
        engine.step(2)
        height = 320 if args.content == 'openarena' else 256
        engine.request('exec', command=f'give health; setviewpos {origin[0]} {origin[1]} {origin[2]+height} 0')
        engine.step(300)
        health = engine.request('state')['player']['health']
        assert -40 < health <= 0, f'ordinary fall death required, health={health}'
        engine.request('exec', command='physics_status')
        engine.step(2)
        capture = engine.request('capture', name='ragdoll')
        engine.step(2)
        shutil.copyfile(engine.base/capture['path'], args.output/'ragdoll.png')
        engine.request('exec', command='map_restart 0')
        engine.step(120)
        engine.request('exec', command='physics_status')
        engine.step(2)
        engine.request('exec', command='disconnect')
        engine.step(2)
    finally:
        shutil.copyfile(engine.log_path, args.output/'death.log')
text = (args.output/'death.log').read_text(errors='replace')
assert 'Physics ragdoll death: owner=0 serial=1 joints=16' in text
assert re.search(r'Physics ragdoll status: prepared=4 spawned=1 active=1 draws=[1-9]\d*', text)
statuses = re.findall(r'Physics ragdoll status: prepared=4 spawned=(\d+) active=(\d+) draws=(\d+)', text)
assert len(statuses) == 3 and statuses[-1][1] == '0', 'restart must retire cosmetic deaths'
storage = re.findall(r'Physics status: steps=\d+ arena=(\d+) allocations=(\d+) live=(\d+)', text)
assert len(storage) == 3 and len(set(storage)) == 1, 'death/restart must not allocate'
assert 'Physics shutdown: live=0' in text
assert 'arena exhausted' not in text and 'contact capacity exceeded' not in text
print('PASS: replicated fall death activates and renders a skeleton ragdoll; restart releases storage')
