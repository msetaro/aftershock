#!/usr/bin/env python3
"""Exercise temporal camera history and cuts on an owned level with moving entities hidden."""
import argparse
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from PIL import Image, ImageChops, ImageStat
from run import ROOT, SCRATCH
from cook import cook
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--models', action='store_true', help='also exercise native body/view-weapon pose history')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-temporal-runtime')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-temporal-runtime-') as temporary:
    root = Path(temporary)
    source = root/'source'
    shutil.copytree(ROOT/'tests/assets/levels', source)
    subprocess.run([sys.executable, 'tools/level', str(source/'two_lane.json'), '--output', str(root/'compiled')], cwd=ROOT, check=True)
    arguments = ['+set', 'r_fbo', '1', '+set', 'r_postProcess', '1', '+set', 'r_taa', '1',
                 '+set', 'r_drawentities', '0', '+set', 'cg_draw2D', '0', '+set', 'cg_drawGun', '0', '+set', 'con_notifytime', '0']
    with Engine(args.binary, args.data, args.content, home=root/'home', arguments=arguments) as engine:
        shutil.copytree(root/'compiled', engine.base, dirs_exist_ok=True)
        if args.models:
            cook(ROOT/'tests/assets/animation/rigs.json', engine.base)
            engine.request('exec', command='set g_animationBody animations/anim_body.asanim')
            engine.request('exec', command='set g_animationRifle animations/anim_rifle.asanim')
        engine.request('session', dt=20, seed=161)
        engine.request('map', name='two_lane')
        engine.step(100)
        def pose(origin, angles=(0, 0, 0)):
            engine.request('camera', mode='pose', origin=origin, angles=list(angles))
        def capture(name):
            result = engine.request('capture', name=name)
            engine.step(2)
            shutil.copyfile(engine.log_path, args.output/'engine.log')
            path = engine.base/result['path']
            shutil.copyfile(path, args.output/(name+'.png'))
            with Image.open(path) as image:
                return image.convert('RGB')
        def active():
            profile = engine.request('profile')
            names = {row['name'] for row in profile['gpu']}
            assert {'camera motion', 'object motion', 'temporal resolve', 'temporal copy'} <= names, names
            assert profile['post']['draws'] > 0 and profile['post']['dropped'] == 0, profile['post']
            return profile
        pose([-100, -160, 100], (45, 20, 0))
        engine.step(32)
        baseline = capture('static')
        assert max(ImageStat.Stat(baseline).stddev) > 10, 'empty or constant output'
        profile = active()
        # Small camera movement uses history; a large move and FOV change reject it.
        for index in range(8):
            pose([-100 + index*2, -160, 100], (45, 20, 0))
            engine.step(1)
        moved = capture('moved')
        assert ImageChops.difference(baseline, moved).getbbox(), 'camera did not move'
        pose([600, 100, 100], (30, 170, 0))
        engine.step(1)
        cut = capture('cut')
        assert max(ImageStat.Stat(cut).stddev) > 10
        engine.request('cvar.set', name='cg_fov', value='60')
        engine.step(1)
        capture('fov-cut')
        engine.request('exec', command='vid_restart')
        engine.step(32)
        capture('restart')
        active()
        if args.models:
            engine.request('camera', mode='player')
            engine.request('cvar.set', name='r_drawentities', value='1')
            engine.request('cvar.set', name='cg_drawGun', value='1')
            engine.request('exec', command='cmd anim ads 1')
            engine.step(32)
            capture('owned-ads')
            first = active()['temporal']
            assert first['matched'] > 0 and first['motionDraws'] > first['reactiveDraws'], first
            engine.request('exec', command='cmd anim fire 1')
            engine.step(8)
            engine.request('exec', command='cmd anim fire 0')
            engine.request('exec', command='cmd anim reload 1')
            engine.step(20)
            capture('owned-reload')
            engine.request('cvar.set', name='cg_thirdPerson', value='1')
            engine.request('exec', command='+forward')
            engine.step(32)
            engine.request('exec', command='-forward')
            capture('owned-body')
            last = active()['temporal']
            assert last['matched'] > 0 and last['motionDraws'] > first['motionDraws'], last
            assert last['reactiveDraws'] > 0 and last['stored'] <= 256 and last['overflow'] == last['dropped'] == 0, last
            # Remove an occluder without a camera cut. Old body color must not trail.
            engine.step(8)
            camera = engine.request('state')['camera']
            forward = camera['forward']
            angles = [-math.degrees(math.asin(max(-1, min(1, forward[2])))),
                      math.degrees(math.atan2(forward[1], forward[0])), 0]
            pose(camera['origin'], angles)
            engine.step(24)
            occluder = capture('occluder')
            engine.request('cvar.set', name='r_drawentities', value='0')
            revealed = capture('revealed')
            engine.step(32)
            background = capture('background')
            a, b, c = occluder.tobytes(), revealed.tobytes(), background.tobytes()
            mask = [i for i in range(0, len(a), 3) if max(abs(a[i+j]-c[i+j]) for j in range(3)) > 40]
            assert len(mask) > 100, 'owned body did not cover a measurable region'
            error = sorted(max(abs(b[i+j]-c[i+j]) for j in range(3)) for i in mask)
            mean_error = sum(error)/len(error)
            p95_error = error[int(.95*(len(error)-1))]
            assert mean_error < 5 and p95_error < 16, (mean_error, p95_error)
            last['disocclusion'] = dict(pixels=len(mask), mean_error=mean_error, p95_error=p95_error)
            (args.output/'models.json').write_text(json.dumps(last, indent=2)+'\n')
        engine.request('cvar.set', name='r_taa', value='0')
        engine.request('exec', command='vid_restart')
        engine.step(32)
        capture('disabled')
        assert not any(row['name'].startswith('temporal') for row in engine.request('profile')['gpu'])
        (args.output/'report.json').write_text(json.dumps(profile, indent=2)+'\n')
print('PASS: temporal camera passes, small motion, camera cut, restart and explicit disable')
