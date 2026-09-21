#!/usr/bin/env python3
"""Record real-clock q3dm17 GPU scopes without copying installed game content."""
import argparse
import json
import os
from pathlib import Path
from run import SCRATCH
import re
import shutil
import statistics
import subprocess
import tempfile
from run import ROOT

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', required=True, type=Path)
parser.add_argument('--icd', required=True, type=Path, help='reference GPU Vulkan ICD; do not use software rendering')
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-lighting-gpu'))
args = parser.parse_args()
paks = sorted(args.data.resolve().glob('*.pk3'))
if not paks or not args.icd.is_file():
    parser.error('installed Quake 3 paks and the reference GPU ICD are required')
args.output.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-lighting-gpu-') as temporary:
    home = Path(temporary)
    base = home/'baseq3'
    base.mkdir()
    for pak in paks:
        (base/pak.name).symlink_to(pak)
    commands = ['set g_synchronousClients 1', 'set fixedtime 20', 'devmap q3dm17', 'wait 40',
                'team spectator', 'wait 10', 'cmd dev_view 488 1096 416 15 270 0', 'wait 200']
    phases = [('baseline', ['dev_light off', 'set r_ssaoStrength 0']),
              ('point', ['dev_light point 488 1096 512 768 1 0.8 0.5 3']),
              ('combined', ['set r_ssaoStrength 1', 'set r_shadowSun 1'])]
    for phase, setup in phases:
        commands += setup + ['wait 200', f'screenshot {phase}']
        for sample in range(100):
            commands += [f'echo sample_{phase}_{sample}', 'vkinfo', 'wait 3']
    commands += ['quit']
    (base/'gpu.cfg').write_text('\n'.join(commands)+'\n')
    env = dict(os.environ, VK_DRIVER_FILES=str(args.icd.resolve()), VK_ICD_FILENAMES=str(args.icd.resolve()))
    with (args.output/'client.log').open('w') as log:
        subprocess.run(['timeout', '120', 'xvfb-run', '-a', str(args.binary.resolve()),
            '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home), '+set', 'net_enabled', '0', '+set', 'sv_pure', '0',
            '+set', 'r_mode', '-1', '+set', 'r_customwidth', '1280', '+set', 'r_customheight', '720', '+set', 'r_fullscreen', '0',
            '+set', 'r_fbo', '1', '+set', 'r_bloom', '1', '+set', 'r_ssao', '1', '+set', 'r_shadowQuality', '2', '+set', 'r_shadowSun', '0',
            '+set', 'cg_draw2D', '0', '+set', 'cg_drawGun', '0', '+set', 'r_swapInterval', '0', '+set', 'com_maxfps', '0',
            '+set', 'con_notifytime', '0', '+set', 's_initsound', '0', '+set', 'cl_autoRecordDemo', '0', '+set', 'dev_tools', '0',
            '+exec', 'gpu.cfg'], cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
    for path in (base/'screenshots').glob('*.tga'):
        shutil.copyfile(path, args.output/path.name)
text = (args.output/'client.log').read_text()
assert not any(error in text for error in ('ERROR:', 'Unknown command', 'Signal caught'))
results = {}
for phase, index, body in re.findall(r'sample_(baseline|point|combined)_(\d+)\n(.*?)(?=sample_|----- Server Shutdown)', text, re.S):
    values = {}
    for name, value in re.findall(r'^gpu ([^:]+): ([0-9.]+) us', body, re.M):
        # All eight bloom blur passes share a profiler label: retain their sum.
        values[name] = values.get(name, 0) + float(value)
    results.setdefault(phase, []).append(values)
report = {'resolution': [1280, 720], 'shadow_quality': 2, 'ssao': 1, 'bloom': 1,
          'warm_frames': 200, 'samples_per_phase': 100,
          'camera': [488, 1096, 416, 15, 270, 0], 'light': [488, 1096, 512, 768, 1, .8, .5, 3],
          'timing': 'real-clock GPU scopes; fixed simulation tick; excludes presentation wait', 'samples': {}}
assert set(results) == {'baseline', 'point', 'combined'}
for phase, rows in results.items():
    assert len(rows) == 100 and all(rows)
    if phase == 'combined':
        assert all({'local shadow', 'sun shadow', 'ssao', 'ssao blur', 'ssao apply'} <= row.keys() for row in rows)
    scopes = {name: [row.get(name, 0) for row in rows] for name in set().union(*rows)}
    scopes['total'] = [sum(row.values()) for row in rows]
    report['samples'][phase] = {name: {'median_us': statistics.median(values), 'p95_us': sorted(values)[94]}
                                for name, values in sorted(scopes.items())}
(args.output/'timings.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
