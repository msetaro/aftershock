#!/usr/bin/env python3
"""Exercise owned animation graphs in the native game and renderer."""
import argparse
import os
from pathlib import Path
from run import SCRATCH
import re
import shutil
import subprocess
import tempfile

from cook import cook
from run import ROOT, build, content_maps, content_settings

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path)
parser.add_argument('--modules', action='store_true')
parser.add_argument('--server-fps', type=int, choices=[20, 100], default=20)
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-animation-runtime'))
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
# This feature belongs to the owned native game, including when using OA art.
if not args.binary:
    args.binary = build(args.output / 'build', ['BUILD_SERVER=0', 'AFTERSHOCK_DEVTOOLS=1',
                        f'USE_RENDERER_DLOPEN={int(args.modules)}']) / 'quake3e.x64'
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if not paks or len(icds) != 1:
    parser.error('installed content and one lavapipe ICD are required')
with tempfile.TemporaryDirectory(prefix='aftershock-animation-live-') as temporary:
    home = Path(temporary)
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    cook(ROOT / 'tests/assets/animation/rigs.json', base)
    (base / 'animation.cfg').write_text('\n'.join([
        'set g_animationBody animations/anim_body.asanim',
        'set g_animationRifle animations/anim_rifle.asanim',
        'set g_animationTrace 1', 'set cg_animationTrace 1',
        'set g_synchronousClients 1', 'set fixedtime 20', f'set sv_fps {args.server_fps}',
        f'devmap {content_maps(args.content)[0]}', 'wait 40',
        'cmd anim ads 1', 'wait 20', 'screenshot anim_ads',
        'cmd anim fire 1', 'wait 8', 'cmd anim fire 0',
        'cmd anim reload 1', 'wait 80', 'screenshot anim_reload',
        'cmd anim ads 0', 'cmd anim sprint 1', 'wait 20',
        'cmd anim jump 1', 'wait 60', 'cmd anim sprint 0',
        '+forward', 'wait 70', '-forward', 'cmd anim aim_up 1',
        'set cg_thirdPerson 1', 'wait 20', 'screenshot anim_body',
        'cmd anim crouch 1', 'wait 20', 'cmd anim prone 1', 'wait 20',
        'cmd anim lean_left 1', 'wait 20', '+right', 'wait 35', '-right', 'wait 80',
        'anim_status', 'wait 3', 'quit']) + '\n')
    command = ['timeout', '60', 'xvfb-run', '-a', str(args.binary.resolve()),
               '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home),
               *content_settings(args.content), '+set', 'net_enabled', '0', '+set', 'sv_pure', '0',
               '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0', '+set', 's_initsound', '0',
               '+set', 'con_notifytime', '0', '+set', 'com_maxfps', '0', '+set', 'cl_autoRecordDemo', '0', '+exec', 'animation.cfg']
    env = dict(os.environ, LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
    log = args.output / 'client.log'
    with log.open('wb') as stream:
        subprocess.run(command, cwd=ROOT, env=env, stdout=stream, stderr=subprocess.STDOUT, check=True)
    text = log.read_text()
    states = set(re.findall(r'Animation server state: owner=0 rig=1 state=(\w+)', text))
    assert {'idle', 'ads', 'fire', 'reload', 'sprint', 'jump'} <= states, states
    body_states = set(re.findall(r'Animation server state: owner=0 rig=0 state=(\w+)', text))
    assert {'idle', 'move', 'turn'} <= body_states, body_states
    turns = re.findall(r'Animation body facing: yaw=([-0-9.]+) view=([-0-9.]+)', text)
    assert turns and all(abs((float(yaw) - float(view) + 180) % 360 - 180) <= 45 for yaw, view in turns), turns
    optics = re.search(r'Animation ADS: samples=(\d+) max_error=([0-9.]+)', text)
    assert optics and int(optics[1]) >= 5 and float(optics[2]) < 0.01, optics
    events = set(re.findall(r'Animation game event: owner=0 rig=\d+ name=(\w+)', text))
    assert {'shot', 'shell_eject', 'magazine_out', 'magazine_in', 'bolt', 'reload_complete', 'footstep'} <= events, events
    server_rows = re.findall(r'Animation server boxes: tick=(\d+) owner=(\d+) hash=([a-f0-9]+)', text)
    server = {(int(t), int(owner)): digest for t, owner, digest in server_rows}
    assert len(set(server_rows)) == len(server), 'one animation tick published multiple transforms'
    client = {(int(t), int(owner)): digest for t, owner, digest in re.findall(
        r'Animation client boxes: tick=(\d+) owner=(\d+) hash=([a-f0-9]+)', text)}
    assert len(client) >= 50 and client.keys() <= server.keys(), (len(server), len(client))
    assert all(server[key] == digest for key, digest in client.items()), 'replicated hit boxes differ'
    assert re.search(r'Animation rendering: body=[1-9]\d* rifle=[1-9]\d*', text)
    assert not any(error in text for error in ('ERROR:', 'Signal caught', 'Animation rejected'))
    for name in ('anim_ads', 'anim_reload', 'anim_body'):
        shutil.copyfile(base / 'screenshots' / (name + '.tga'), args.output / (name + '.tga'))
print('PASS: owned rifle/body gameplay events, rendered poses and replicated hit-box coordinates')
