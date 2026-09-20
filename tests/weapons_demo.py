#!/usr/bin/env python3
"""Replay the owned weapon fixture against its recorded full authoritative state."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from cook import cook
from run import ROOT, build, content_maps, content_settings


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def states(text, side):
    return {f'{tick}/{hand}': value for hand, tick, value in re.findall(
        rf'Weapon {side} digest: owner=0 hand=(\d+) tick=(\d+) hash=([a-f0-9]{{64}})', text)}


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path)
parser.add_argument('--modules', action='store_true')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-weapons-demo'))
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--record-fixture', action='store_true', help='explicitly record only this content set’s #11 fixture')
args = parser.parse_args()
if args.record_fixture and os.environ.get('CI'):
    parser.error('CI must never record fixtures')
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
if not args.binary:
    args.binary = build(args.output / 'build', ['BUILD_SERVER=0', f'USE_RENDERER_DLOPEN={int(args.modules)}']) / 'quake3e.x64'
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if not paks or len(icds) != 1:
    parser.error('installed content and one lavapipe ICD are required')
fixture_dir = ROOT / 'tests/golden/weapons' / args.content
fixture = fixture_dir / 'range.dm_68'
manifest_path = fixture_dir / 'range.json'
if not args.record_fixture and not manifest_path.is_file():
    parser.error('fixed weapon fixture is absent; explicit initial recording/review is required')
env = dict(os.environ, LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
map_name = content_maps(args.content)[0]


def prepare(home):
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    (base / 'demos').mkdir()
    cook(ROOT / 'tests/assets/range.json', base)
    cook(ROOT / 'tests/assets/weapons/assets.json', base)
    source = home / 'source'
    source.mkdir()
    rifle = json.loads((ROOT / 'tests/assets/weapons/rifle.weapon.json').read_text())
    grenade = dict(rifle, name='range_grenade', ballistics='projectile', fire_mode='semi', damage=20,
                   magazine=4, reserve=8, projectile=dict(rifle['projectile'], speed=320, fuse_ms=1000))
    (source / 'grenade.json').write_text(json.dumps(grenade))
    recipe = source / 'assets.json'
    recipe.write_text(json.dumps({'version': 1, 'assets': [
        {'name': 'weapons/grenade', 'kind': 'weapon', 'source': 'grenade.json'}]}))
    cook(recipe, base)
    return base


def assets(base):
    return {str(path.relative_to(base)): digest(path)
            for pattern in ('animations/*.asanim', 'models/*.iqm', 'weapons/*.asweapon', 'sound/*.wav', 'effects/*.asmat')
            for path in sorted(base.glob(pattern))}


def client(home, commands, name, recording=False):
    clock = [] if recording else ['faketime', '-f', '@2026-01-01 00:00:00 i0.01']
    command = ['timeout', '90', 'xvfb-run', '-a', *clock, str(args.binary.resolve()),
        '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home), *content_settings(args.content),
        '+set', 'net_enabled', '0', '+set', 'sv_pure', '0', '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0',
        '+set', 's_initsound', '0', '+set', 'con_notifytime', '0', '+set', 'com_maxfps', '0',
        '+set', 'cl_autoRecordDemo', '0', '+set', 'cg_weaponTrace', '1', *commands]
    result = subprocess.run(command, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    text = result.stdout.decode(errors='replace')
    (args.output / (name + '.log')).write_text(text)
    result.check_returncode()
    assert 'VK_RENDERER:' in text and 'llvmpipe' in text and 'Static cgame loaded.' in text
    assert not any(error in text for error in ('ERROR:', 'Signal caught', 'Weapon rejected', 'unknown cmd'))
    return text


if args.record_fixture:
    with tempfile.TemporaryDirectory(prefix='aftershock-weapons-record-') as temporary:
        home = Path(temporary)
        base = prepare(home)
        commands = [
            'set g_weapons "weapons/range_rifle.asweapon weapons/grenade.asweapon"',
            'set g_weaponTrace 1', 'set g_synchronousClients 1', 'set fixedtime 20', 'set sv_fps 50',
            f'devmap {map_name}', 'wait 60', 'cmd weapon_attachment 0 1', 'wait 15',
            'record range', 'wait 20', '+attack', 'wait 130', '-attack', '+button12', 'wait 30',
            '+button13', 'wait 2', '-button13', 'wait 65', '-button12', '+button14', 'wait 30', '-button14',
            '+button15', 'wait 15', '-button15', 'weapon 2', 'wait 20', '+attack', 'wait 2', '-attack',
            'wait 80', 'weapon 1', 'wait 20', '+attack', 'wait 25', '-attack', 'wait 60',
            'stoprecord', 'quit']
        (base / 'weapons-record.cfg').write_text('\n'.join(commands) + '\n')
        log = client(home, ['+exec', 'weapons-record.cfg'], 'record', recording=True)
        authoritative, received = states(log, 'server'), states(log, 'client')
        assert len(received) >= 300 and all(authoritative[key] == value for key, value in received.items())
        assert 'Weapon projectile exploded: owner=0 sequence=1 ' in log
        assert all(f'Weapon event: owner=0 hand=0 kind={kind}' in log for kind in (0, 2, 3))
        fixture_dir.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(base / 'demos/range.dm_68', fixture)
        manifest = {'version': 1, 'content': args.content, 'map': map_name, 'demo_sha256': digest(fixture),
                    'recorded_revision': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                    'commands': commands, 'server_states': authoritative, 'assets': assets(base)}
        manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
manifest = json.loads(manifest_path.read_text())
assert manifest['version'] == 1 and manifest['content'] == args.content and manifest['map'] == map_name
assert digest(fixture) == manifest['demo_sha256'], 'fixed weapon demo differs from its reviewed manifest'
results, traces = [], []
for iteration in (1, 2):
    with tempfile.TemporaryDirectory(prefix='aftershock-weapons-replay-') as temporary:
        home = Path(temporary)
        base = prepare(home)
        assert assets(base) == manifest['assets'], 'weapon/graph/model revision changed'
        shutil.copyfile(fixture, base / 'demos/range.dm_68')
        log = client(home, ['+set', 'timedemo', '1', '+demo', 'range',
                           '+wait', '50', '+screenshot', 'frame050', '+wait', '50', '+screenshot', 'frame100',
                           '+wait', '100', '+screenshot', 'frame200', '+wait', '2', '+weapon_status', '+wait', '400', '+quit'], f'replay-{iteration}')
        received = states(log, 'client')
        assert len(received) >= 300 and received.keys() <= manifest['server_states'].keys(), len(received)
        assert all(manifest['server_states'][key] == value for key, value in received.items()), 'recorded full weapon state differs'
        animation = set(re.findall(r'Weapon animation client: owner=0 hand=0 state=(\w+)', log))
        assert {'idle', 'ads', 'fire', 'reload', 'melee'} <= animation, animation
        assert 'Weapon projectile client: owner=0 hand=0 sequence=1 ' in log
        assert re.search(r'Weapon rendering: draws=[1-9]\d* attachments=[1-9]\d*', log)
        frames = {}
        for name in ('frame050', 'frame100', 'frame200'):
            target = args.output / f'{iteration}-{name}.tga'
            shutil.copyfile(base / 'screenshots' / (name + '.tga'), target)
            frames[name] = digest(target)
        results.append(frames)
        traces.append(received)
assert results[0] == results[1], 'fixed weapon replay frames differ'
assert traces[0] == traces[1], 'fixed weapon replay state differs'
(args.output / 'replay.json').write_text(json.dumps({'frames': results[0], 'states': len(traces[0])}, indent=2) + '\n')
print(f'PASS: fixed {args.content} weapon replay; {len(traces[0])} authoritative full-state hashes and three repeatable frames')
