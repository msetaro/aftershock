#!/usr/bin/env python3
"""Measure opt-in SSAO, bloom/MSAA composition and restart on an owned level."""
import argparse
import os
from pathlib import Path
from run import SCRATCH
import shutil
import subprocess
import sys
import tempfile
from PIL import Image
from run import ROOT, content_settings

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', required=True, type=Path)
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--content', choices=('quake3', 'openarena'), default='quake3')
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-ssao-runtime'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if not paks or len(icds) != 1:
    parser.error('installed content and one lavapipe ICD are required')
with tempfile.TemporaryDirectory(prefix='aftershock-ssao-') as temporary:
    home = Path(temporary)
    base = home/('baseoa' if args.content == 'openarena' else 'baseq3')
    subprocess.run([sys.executable, 'tools/level', 'tests/assets/levels/two_lane.json',
                    '--output', str(base)], cwd=ROOT, check=True)
    for pak in paks:
        (base/pak.name).symlink_to(pak)
    view = ['cmd dev_view -160 0 96 10 0 0', 'wait 10']
    for quality, samples, bloom in ((1, 0, 0), (2, 0, 0), (1, 4, 1)):
        label = f'q{quality}-ms{samples}-b{bloom}'
        commands = ['set g_synchronousClients 1', 'set fixedtime 20', 'devmap two_lane', 'wait 40',
                    'team spectator', 'wait 10', *view]
        for name, strength in (('off', 0), ('on', 2), ('restored', 0)):
            commands += [f'set r_ssaoStrength {strength}', 'wait 4', f'screenshot {name}', 'wait 3']
        commands += ['set r_ssaoStrength 2', 'vid_restart', 'wait 40', *view,
                     'screenshot restarted', 'wait 3', 'quit']
        (base/'ssao.cfg').write_text('\n'.join(commands)+'\n')
        log = args.output/(label+'.log')
        env = dict(os.environ, LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
        with log.open('w') as stream:
            subprocess.run(['timeout', '90', 'xvfb-run', '-a', 'faketime', '-f', '@2026-01-01 00:00:00 i0.01', str(args.binary.resolve()),
                '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home), *content_settings(args.content),
                '+set', 'net_enabled', '0', '+set', 'sv_pure', '0', '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0',
                '+set', 'r_gamma', '1', '+set', 'r_intensity', '1', '+set', 'r_overBrightBits', '0',
                '+set', 'r_fbo', '1', '+set', 'r_ssao', str(quality), '+set', 'r_ssaoRadius', '48',
                '+set', 'r_ext_multisample', str(samples), '+set', 'r_bloom', str(bloom),
                '+set', 'r_shadowQuality', '1', '+set', 'r_stencilbits', '8',
                '+set', 'con_notifytime', '0', '+set', 'cg_draw2D', '0', '+set', 'cg_drawGun', '0',
                '+set', 's_initsound', '0', '+set', 'com_maxfps', '0', '+set', 'cl_autoRecordDemo', '0',
                '+set', 'dev_tools', '0', '+exec', 'ssao.cfg'], cwd=ROOT, env=env,
                stdout=stream, stderr=subprocess.STDOUT, check=True)
        text = log.read_text()
        assert not any(word in text for word in ('ERROR:', 'Signal caught', 'Unknown command'))
        captures = {}
        for name in ('off', 'on', 'restored', 'restarted'):
            path = base/'screenshots'/(name+'.tga')
            shutil.copyfile(path, args.output/(label+'-'+path.name))
            with Image.open(path) as image:
                captures[name] = image.convert('RGB').crop((20, 160, 620, 420)).tobytes()
            path.unlink()
        assert captures['off'] == captures['restored'], 'SSAO toggle changed the baseline'
        assert captures['on'] == captures['restarted'], 'SSAO restart changed the image'
        changed = sum(a != b for a, b in zip(captures['off'], captures['on']))
        assert changed > 1000, ('SSAO did not affect contact regions', changed)
        assert sum(captures['on']) < sum(captures['off']), 'SSAO must attenuate contact lighting'
        print(f'PASS: {label}, {changed} changed channel bytes; exact disable/restart', flush=True)
