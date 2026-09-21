#!/usr/bin/env python3
"""Replay an owned animation demo against its recorded authoritative hit boxes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
from run import SCRATCH
import re
import shutil
import subprocess
import tempfile

from cook import cook
from run import ROOT, build, content_maps, content_settings


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def boxes(text, side):
    return {f'{tick}/{owner}': value for tick, owner, value in re.findall(
        rf'Animation {side} boxes: tick=(\d+) owner=(\d+) hash=([a-f0-9]+)', text)}


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path)
parser.add_argument('--modules', action='store_true')
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-animation-demo'))
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--record-fixture', action='store_true', help='explicitly record and replace only this content set’s #10 fixture')
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
fixture_dir = ROOT / 'tests/golden/animation' / args.content
fixture = fixture_dir / 'rifle-body.dm_68'
manifest_path = fixture_dir / 'rifle-body.json'
if not args.record_fixture and not manifest_path.is_file():
    parser.error('fixed animation fixture is absent; explicit initial recording/review is required')
env = dict(os.environ, LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
map_name = content_maps(args.content)[0]


def prepare(home):
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    (base / 'demos').mkdir()
    cook(ROOT / 'tests/assets/animation/rigs.json', base)
    return base


def client(home, commands, name, recording=False):
    clock = [] if recording else ['faketime', '-f', '@2026-01-01 00:00:00 i0.01']
    command = ['timeout', '90', 'xvfb-run', '-a', *clock, str(args.binary.resolve()),
        '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home), *content_settings(args.content),
        '+set', 'net_enabled', '0', '+set', 'sv_pure', '0', '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0',
        '+set', 's_initsound', '0', '+set', 'con_notifytime', '0', '+set', 'com_maxfps', '0',
        '+set', 'cl_autoRecordDemo', '0', '+set', 'cg_animationTrace', '1', *commands]
    result = subprocess.run(command, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    text = result.stdout.decode(errors='replace')
    (args.output / (name + '.log')).write_text(text)
    result.check_returncode()
    assert 'VK_RENDERER:' in text and 'llvmpipe' in text and 'Static cgame loaded.' in text
    assert not any(error in text for error in ('ERROR:', 'Signal caught', 'Animation rejected', 'unknown cmd'))
    return text


if args.record_fixture:
    with tempfile.TemporaryDirectory(prefix='aftershock-animation-record-') as temporary:
        home = Path(temporary)
        base = prepare(home)
        commands = [
            'set g_animationBody animations/anim_body.asanim', 'set g_animationRifle animations/anim_rifle.asanim',
            'set g_animationTrace 1', 'set g_synchronousClients 1', 'set fixedtime 20',
            f'devmap {map_name}', 'wait 30', 'record rifle-body', 'wait 20',
            'cmd anim ads 1', 'wait 60', 'cmd anim fire 1', 'wait 20',
            'cmd anim reload 1', 'wait 80', 'cmd anim ads 0', 'cmd anim sprint 1', 'wait 60',
            'cmd anim jump 1', 'wait 60', 'cmd anim sprint 0', '+forward', 'wait 80', '-forward',
            'set cg_thirdPerson 1', 'cmd anim aim_up 1', 'wait 20', 'cmd anim crouch 1', 'wait 40',
            'cmd anim prone 1', 'wait 40', 'cmd anim lean_left 1', 'wait 40', '+right', 'wait 35', '-right',
            'wait 80', 'stoprecord', 'quit']
        (base / 'animation-record.cfg').write_text('\n'.join(commands) + '\n')
        log = client(home, ['+exec', 'animation-record.cfg'], 'record', recording=True)
        authoritative = boxes(log, 'server')
        received = boxes(log, 'client')
        assert len(received) >= 100 and all(authoritative[key] == value for key, value in received.items())
        fixture_dir.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(base / 'demos/rifle-body.dm_68', fixture)
        manifest = {'version': 1, 'content': args.content, 'map': map_name, 'demo_sha256': digest(fixture),
                    'recorded_revision': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                    'commands': commands, 'server_boxes': authoritative,
                    'assets': {str(path.relative_to(base)): digest(path) for pattern in ('animations/*.asanim', 'models/anim_*.iqm')
                               for path in sorted(base.glob(pattern))}}
        manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
manifest = json.loads(manifest_path.read_text())
assert manifest['version'] == 1 and manifest['content'] == args.content and manifest['map'] == map_name
assert digest(fixture) == manifest['demo_sha256'], 'fixed animation demo differs from its reviewed manifest'
results, poses = [], []
for iteration in (1, 2):
    with tempfile.TemporaryDirectory(prefix='aftershock-animation-replay-') as temporary:
        home = Path(temporary)
        base = prepare(home)
        assert all(digest(base / path) == value for path, value in manifest['assets'].items()), 'graph/model revision changed'
        shutil.copyfile(fixture, base / 'demos/rifle-body.dm_68')
        log = client(home, ['+set', 'timedemo', '1', '+demo', 'rifle-body',
                           '+wait', '50', '+screenshot', 'frame050', '+wait', '50', '+screenshot', 'frame100',
                           '+wait', '100', '+screenshot', 'frame200', '+wait', '2', '+anim_status', '+wait', '400', '+quit'], f'replay-{iteration}')
        received = boxes(log, 'client')
        assert len(received) >= 150 and received.keys() <= manifest['server_boxes'].keys(), len(received)
        assert all(manifest['server_boxes'][key] == value for key, value in received.items()), 'recorded server/client hit boxes differ'
        states = set(re.findall(r'Animation client state: owner=0 rig=1 state=(\w+)', log))
        assert {'idle', 'ads', 'fire', 'reload', 'sprint', 'jump'} <= states, states
        body_states = set(re.findall(r'Animation client state: owner=0 rig=0 state=(\w+)', log))
        assert {'idle', 'move', 'turn'} <= body_states, body_states
        assert re.search(r'Animation rendering: body=[1-9]\d* rifle=[1-9]\d*', log)
        frames = {}
        for name in ('frame050', 'frame100', 'frame200'):
            target = args.output / f'{iteration}-{name}.tga'
            shutil.copyfile(base / 'screenshots' / (name + '.tga'), target)
            frames[name] = digest(target)
        assert len(set(frames.values())) == 3, 'replay frames did not advance'
        results.append(frames)
        poses.append(received)
assert results[0] == results[1] and poses[0] == poses[1], 'fixed replay is not repeatable'
(args.output / 'replay.json').write_text(json.dumps({'demo_sha256': digest(fixture), 'frames': results[0],
                                                  'received_server_box_matches': len(poses[0])}, indent=2) + '\n')
print(f'PASS: fixed {args.content} animation demo; {len(poses[0])} authoritative hit-box hashes and repeated sampled frames agree')
