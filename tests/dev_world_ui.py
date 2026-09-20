#!/usr/bin/env python3
"""Drive entity spawn/picking and collision/navigation controls through real ImGui input."""
import argparse
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from run import ROOT, build, content_maps, content_settings
from window import wait_for, XInput

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path)
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-dev-world-ui'))
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--inside-xvfb', action='store_true', help=argparse.SUPPRESS)
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
if not args.binary:
    args.binary = build(args.output / 'build', ['BUILD_SERVER=0', 'AFTERSHOCK_DEVTOOLS=1']) / 'quake3e.x64'
if not args.inside_xvfb:
    subprocess.run(['timeout', '90', 'xvfb-run', '-a', sys.executable, str(Path(__file__).resolve()),
                    '--inside-xvfb', '--binary', str(args.binary.resolve()), '--output', str(args.output),
                    '--data', str(args.data.resolve()), '--content', args.content], cwd=ROOT, check=True)
    raise SystemExit(0)
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if not paks or len(icds) != 1:
    parser.error('installed content and one lavapipe ICD are required')
map_name = content_maps(args.content)[0]
with tempfile.TemporaryDirectory(prefix='aftershock-world-ui-') as temporary:
    home = Path(temporary)
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    (base / 'world-ui.cfg').write_text('\n'.join([
        f'devmap {map_name}', 'wait 10', 'echo entity_ui_ready', 'wait 100',
        'screenshot entities', 'devtools_status', 'echo world_ui_ready', 'wait 100',
        'screenshot world', 'wait 2', 'devtools_status', 'quit']) + '\n')
    command = [str(args.binary.resolve()), '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home),
               *content_settings(args.content), '+set', 'net_enabled', '0', '+set', 'sv_pure', '0',
               '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0', '+set', 's_initsound', '0',
               '+set', 'dev_tools', '1', '+set', 'fixedtime', '50', '+set', 'com_maxfps', '20',
               '+set', 'cl_autoRecordDemo', '0', '+exec', 'world-ui.cfg']
    env = dict(os.environ, VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]), LP_NUM_THREADS='1')
    log_path = args.output / 'client.log'
    with log_path.open('wb') as log:
        process = subprocess.Popen(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
        input_device = XInput()
        try:
            wait_for(lambda: b'entity_ui_ready' in log_path.read_bytes(), process)
            input_device.verify_window(process)
            for x, y in ((500, 64), (55, 268), (125, 268), (70, 90)):
                input_device.click(x, y)
            wait_for(lambda: b'world_ui_ready' in log_path.read_bytes(), process)
            for x, y in ((560, 64), (25, 90), (25, 113), (25, 136)):
                input_device.click(x, y)
            process.wait(timeout=30)
            assert process.returncode == 0
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            input_device.close()
    for name in ('entities', 'world'):
        image = base / 'screenshots' / (name + '.tga')
        assert struct.unpack_from('<HH', image.read_bytes(), 12) == (640, 480)
        shutil.copyfile(image, args.output / image.name)
text = log_path.read_text()
assert not any(error in text for error in ('ERROR:', 'Signal caught', 'capacity exceeded', 'rejected', 'Unknown command'))
spawned = re.search(r'Developer entity action 3: completed \(entity (\d+)\)', text)
picked = re.search(r'Developer entity action 6: completed \(entity (\d+)\)', text)
assert spawned and picked and spawned[1] == picked[1], 'spawn/pick UI did not select the new entity'
assert 'collision=1 navigation=1' in text
samples = [tuple(map(int, row)) for row in re.findall(r'Developer drawing: lines=(\d+) labels=(\d+)', text)]
assert len(samples) == 2 and samples[1][0] > 12 and all(row[1] > 0 for row in samples)
print('PASS: real UI spawn, world picking, collision/navigation wireframes and projected entity label')
