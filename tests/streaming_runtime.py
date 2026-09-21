#!/usr/bin/env python3
"""Stream an owned 4K texture set larger than its residency budget on a generated level."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from PIL import Image
from cook import cook
from run import ROOT, SCRATCH
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-streaming-runtime')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
source = args.output/'source'
source.mkdir(exist_ok=True)
roles = ('floor', 'wall', 'trim', 'cover', 'prop')
colors = ((180, 100, 60), (80, 140, 180), (160, 170, 80), (120, 80, 160), (80, 160, 100))
for name, color in zip(roles, colors):
    Image.new('RGB', (4096, 4096), color).save(source/(name+'.png'))
project = source/'assets.json'
project.write_text(json.dumps({'version': 1, 'assets': [
    dict(name='textures/level/'+name, kind='texture', source=name+'.png', format='bc7', srgb=True)
    for name in roles]}))
cook(project, args.output/'cooked')
source_bytes = sum(path.stat().st_size for path in (args.output/'cooked/textures/level').glob('*.ktx2'))
assert source_bytes > 32*1024*1024
subprocess.run([sys.executable, 'tools/level', str(ROOT/'tests/assets/levels/two_lane.json'),
                '--output', str(args.output/'compiled')], cwd=ROOT, check=True)
arguments = ['+set', 'r_textureStreaming', '1', '+set', 'r_textureBudgetMB', '32',
             '+set', 'r_textureSourceMB', '128', '+set', 'r_drawentities', '0',
             '+set', 'cg_draw2D', '0', '+set', 'cg_drawGun', '0', '+set', 'con_notifytime', '0',
             '+set', 'r_mode', '-1', '+set', 'r_customwidth', '320', '+set', 'r_customheight', '240']
profiles = []
with tempfile.TemporaryDirectory(prefix='aftershock-streaming-') as temporary:
    with Engine(args.binary, args.data, args.content, home=Path(temporary), arguments=arguments) as engine:
        shutil.copytree(args.output/'compiled', engine.base, dirs_exist_ok=True)
        shutil.copytree(args.output/'cooked', engine.base, dirs_exist_ok=True)
        engine.request('session', dt=20, seed=161)
        engine.request('map', name='two_lane')
        engine.step(160)

        def sample():
            shutil.copyfile(engine.log_path, args.output/'engine.log')
            profile = engine.request('profile')
            stats = profile['textureStreaming']
            assert stats['images'] == len(roles), stats
            assert 0 < stats['usedBytes'] <= stats['peakBytes'] <= stats['budgetBytes'] == 32*1024*1024, stats
            assert stats['sourceBytes'] > stats['budgetBytes'] and stats['sourceBytes'] <= stats['sourceBudgetBytes'], stats
            assert stats['failures'] == 0, stats
            profiles.append(profile)
            return stats

        first = sample()
        assert first['promotions'] > 0, first
        for x, yaw in ((0, 0), (768, 90), (1536, 180), (768, -90), (0, 180)):
            engine.request('camera', mode='pose', origin=[x, 0, 100], angles=[0, yaw, 0])
            engine.step(160)
            sample()
        engine.request('cvar.set', name='r_drawworld', value='0')
        engine.step(180)
        cold = sample()
        assert cold['demotions'] > 0 and cold['fullResolution'] == 0, cold
        engine.request('cvar.set', name='r_drawworld', value='1')
        engine.step(180)
        hot = sample()
        assert hot['promotions'] > cold['promotions'] and hot['fullResolution'] > 0, hot
        capture = engine.request('capture', name='streamed')
        engine.step(2)
        shutil.copyfile(engine.base/capture['path'], args.output/'streamed.png')
        engine.request('exec', command='vid_restart')
        engine.step(180)
        sample()
        shutil.copyfile(engine.log_path, args.output/'engine.log')
(args.output/'report.json').write_text(json.dumps(dict(source_file_bytes=source_bytes, profiles=profiles), indent=2)+'\n')
print('PASS: owned 4K set exceeds VRAM budget, promotes/evicts, retains coarse tails and survives restart')
