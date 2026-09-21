#!/usr/bin/env python3
"""Exercise temporal camera history and cuts on an owned level with moving entities hidden."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from PIL import Image, ImageChops, ImageStat
from run import ROOT, SCRATCH
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
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
        engine.request('cvar.set', name='r_taa', value='0')
        engine.request('exec', command='vid_restart')
        engine.step(32)
        capture('disabled')
        assert not any(row['name'].startswith('temporal') for row in engine.request('profile')['gpu'])
        (args.output/'report.json').write_text(json.dumps(profile, indent=2)+'\n')
print('PASS: temporal camera passes, small motion, camera cut, restart and explicit disable')
