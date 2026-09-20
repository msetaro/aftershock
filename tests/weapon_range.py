#!/usr/bin/env python3
"""Exercise target-range controls through real ImGui input on a private display."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

from cook import cook
from run import ROOT, content_maps, content_settings
from window import XInput, wait_for

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-weapon-range'))
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--inside-xvfb', action='store_true', help=argparse.SUPPRESS)
args = parser.parse_args()
if not args.inside_xvfb:
    subprocess.run(['timeout', '90', 'xvfb-run', '-a', sys.executable, str(Path(__file__).resolve()),
                    '--inside-xvfb', '--binary', str(args.binary.resolve()), '--output', str(args.output.resolve()),
                    '--content', args.content, '--data', str(args.data.resolve())], cwd=ROOT, check=True)
    raise SystemExit(0)
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if not paks or len(icds) != 1:
    parser.error('installed content and one lavapipe ICD are required')
with tempfile.TemporaryDirectory(prefix='aftershock-range-ui-') as temporary:
    home = Path(temporary)
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    sources = home / 'sources'
    sources.mkdir()
    definition = json.loads((ROOT / 'tests/assets/weapons/rifle.weapon.json').read_text())
    (sources / 'rifle.json').write_text(json.dumps(definition))
    (sources / 'second.json').write_text(json.dumps(dict(definition, name='range_second', damage=55)))
    project = {'version': 1, 'assets': [{'name': 'weapons/' + name, 'kind': 'weapon', 'source': source}
               for name, source in [('range_rifle', 'rifle.json'), ('second', 'second.json')]]}
    (sources / 'assets.json').write_text(json.dumps(project))
    cook(sources / 'assets.json', base)
    cook(ROOT / 'tests/assets/range.json', base)
    (base / 'range.cfg').write_text('\n'.join([
        'set g_weapons "weapons/range_rifle.asweapon weapons/second.asweapon"',
        'set g_rewind 1', 'set g_weaponTrace 1', 'set cg_weaponTrace 1',
        f'devmap {content_maps(args.content)[0]}', 'wait 30', 'dev_weapon_range',
        'bind F10 "screenshot range;wait 2;quit"']) + '\n')
    env = dict(os.environ, SDL_AUDIODRIVER='dummy', LP_NUM_THREADS='1',
               VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
    logfile = args.output / 'client.log'
    with logfile.open('wb') as log:
        process = subprocess.Popen([str(args.binary.resolve()), '+set', 'fs_basepath', str(home),
            '+set', 'fs_homepath', str(home), *content_settings(args.content),
            '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0', '+set', 's_initsound', '1',
            '+set', 'com_maxfps', '50', '+set', 'net_enabled', '0', '+set', 'sv_pure', '0',
            '+set', 'cl_autoRecordDemo', '0', '+exec', 'range.cfg'],
            cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
        device = XInput()
        try:
            wait_for(lambda: 'Developer weapon range: opened' in logfile.read_text(), process, seconds=15)
            device.verify_window(process)
            time.sleep(0.5)
            # Stable rows within the selected range panel.
            device.click(90, 180)  # Spawn moving target.
            wait_for(lambda: 'Rewind target created:' in logfile.read_text(), process)
            device.click(290, 180)  # Second data-only rifle.
            wait_for(lambda: 'Weapon switch: owner=0 hand=0 from=0 to=1' in logfile.read_text(), process)
            device.click(35, 204)  # One trigger pulse.
            wait_for(lambda: 'Weapon event: owner=0 hand=0 kind=0' in logfile.read_text(), process)
            device.click(90, 204)  # Reload.
            wait_for(lambda: 'Weapon event: owner=0 hand=0 kind=4' in logfile.read_text(), process)
            time.sleep(1.2)
            device.key('Escape')
            device.key('F10')
            process.wait(timeout=20)
            assert process.returncode == 0
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            device.close()
    assert 'Developer weapon range: target' in logfile.read_text()
    assert 'Developer weapon range: fire' in logfile.read_text()
    shutil.copyfile(base / 'screenshots/range.tga', args.output / 'range.tga')
print('PASS: real ImGui target spawn, data-only rifle selection, trigger and reload')
