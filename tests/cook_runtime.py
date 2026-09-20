#!/usr/bin/env python3
"""Render the owned Blender character and measure a watched texture edit on screen."""
import argparse
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import struct
import sys
import tempfile
import time

from PIL import Image, ImageChops
from run import ROOT, build, content_maps, content_settings
from window import XInput, wait_for

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path)
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-cook-runtime-test'))
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
with tempfile.TemporaryDirectory(prefix='aftershock-cook-live-') as temporary:
    home = Path(temporary)
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    source = home / 'source'
    shutil.copytree(ROOT / 'tests/assets/cook-character', source)
    watcher_log = args.output / 'watch.log'
    client_log = args.output / 'client.log'
    with watcher_log.open('wb') as watch_log:
        watcher = subprocess.Popen([sys.executable, 'tools/cook', str(source / 'assets.json'), '--output', str(base), '--watch'], cwd=ROOT, stdout=watch_log, stderr=subprocess.STDOUT)
        try:
            wait_for(lambda: (base / 'cook.revision').is_file(), watcher)
            revision = (base / 'cook.revision').read_bytes()
            (base / 'cook-test.cfg').write_text('\n'.join([
                f'devmap {content_maps(args.content)[0]}', 'wait 10', 'set dev_reloadAssets 1', 'dev_reloadAssets', 'set timescale 0',
                'echo cook_ui_ready', 'wait 200', 'imagelist', 'modellist',
                'screenshot before', 'wait 3', 'echo cook_edit', 'wait 12',
                'screenshot after', 'wait 4', 'devtools_status', 'echo cook_wave',
                'wait 100', 'screenshot wave', 'devtools_status', 'echo cook_model_edit',
                'wait 80', 'screenshot model_reload', 'devtools_status', 'echo cook_idle',
                'wait 80', 'screenshot idle', 'devtools_status', 'wait 2', 'quit']) + '\n')
            command = [str(args.binary.resolve()), '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home),
                       *content_settings(args.content), '+set', 'net_enabled', '0', '+set', 'sv_pure', '0',
                       '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0', '+set', 's_initsound', '0',
                       '+set', 'dev_tools', '1', '+set', 'dev_reloadAssets', '1', '+set', 'com_maxfps', '20',
                       '+set', 'cl_autoRecordDemo', '0', '+exec', 'cook-test.cfg']
            env = dict(os.environ, LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
            with client_log.open('wb') as log:
                process = subprocess.Popen(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
                input_device = XInput()
                try:
                    wait_for(lambda: b'cook_ui_ready' in client_log.read_bytes(), process)
                    input_device.verify_window(process)
                    input_device.click(440, 64)
                    input_device.click(120, 91)
                    for char in 'models/character.iqm':
                        input_device.key({'/': 'slash', '.': 'period'}.get(char, char))
                    input_device.click(80, 137)
                    time.sleep(0.3)
                    input_device.click(320, 316)
                    wait_for(lambda: b'cook_edit' in client_log.read_bytes(), process)
                    edited = time.monotonic()
                    Image.new('RGBA', (16, 16), (32, 240, 64, 255)).save(source / 'character.png')
                    after = base / 'screenshots/after.tga'
                    wait_for(after.is_file, process)
                    latency = time.monotonic() - edited
                    wait_for(lambda: b'cook_wave' in client_log.read_bytes(), process)
                    input_device.click(100, 227)
                    input_device.click(80, 274)
                    input_device.click(220, 270)
                    wait_for(lambda: b'cook_model_edit' in client_log.read_bytes(), process)
                    document = json.loads((source / 'character.gltf').read_text())
                    wave = next(clip for clip in document['animations'] if clip['name'] == 'wave')
                    channel = next(channel for channel in wave['channels'] if channel['target']['path'] == 'rotation'
                                   and document['nodes'][channel['target']['node']]['name'] == 'arm.L')
                    accessor = document['accessors'][wave['samplers'][channel['sampler']]['output']]
                    assert accessor['componentType'] == 5126 and accessor['type'] == 'VEC4'
                    view = document['bufferViews'][accessor['bufferView']]
                    buffer_path = source / document['buffers'][view['buffer']]['uri']
                    data = bytearray(buffer_path.read_bytes())
                    offset = view.get('byteOffset', 0) + accessor.get('byteOffset', 0)
                    stride = view.get('byteStride', 16)
                    first = bytes(data[offset:offset + 16])
                    for frame in range(accessor['count']):
                        data[offset + frame * stride:offset + frame * stride + 16] = first
                    for bound in ('min', 'max'):
                        if bound in accessor:
                            accessor[bound] = list(struct.unpack('<4f', first))
                    wave['name'] = 'salute'
                    temporary_buffer = buffer_path.with_suffix('.tmp')
                    temporary_buffer.write_bytes(data)
                    temporary_buffer.replace(buffer_path)
                    temporary_gltf = source / 'character.tmp'
                    temporary_gltf.write_text(json.dumps(document))
                    temporary_gltf.replace(source / 'character.gltf')
                    wait_for(lambda: b'cook_idle' in client_log.read_bytes(), process)
                    input_device.click(100, 227)
                    input_device.click(80, 255)
                    input_device.click(220, 270)
                    process.wait(timeout=20)
                    assert process.returncode == 0
                    assert (base / 'cook.revision').read_bytes() != revision
                    text = client_log.read_text()
                    assert 'Cooked texture reloaded:' in text, 'renderer did not consume the new cooked revision'
                    poses = re.findall(r'Developer animation: model=(\d+) frame=(\d+) previews=\d+ clip=(\w+)', text)
                    assert len(poses) == 4 and poses[0][2] == 'idle' and len({pose[0] for pose in poses}) == 1, poses
                    assert poses[1][2] == 'wave' and 31 < int(poses[1][1]) < 61, poses
                    assert poses[2][2] == 'salute' and poses[2][1] == poses[1][1], poses
                    assert 'Cooked model reloaded:' in text
                    assert poses[3][2] == 'idle' and 0 < int(poses[3][1]) < 30, poses
                    assert 'BC7s' in text and 'models/character' in text
                    assert not any(message in text for message in ('ERROR:', 'Signal caught', 'Invalid or unavailable', 'Invalid or unsupported'))
                    before = Image.open(base / 'screenshots/before.tga').convert('RGB')
                    after_image = Image.open(after).convert('RGB')
                    # Compare the model preview only, excluding counters and the scene.
                    delta = ImageChops.difference(before.crop((18, 335, 621, 460)), after_image.crop((18, 335, 621, 460)))
                    pixels = delta.tobytes()
                    changed = sum(max(pixels[i:i + 3]) > 40 for i in range(0, len(pixels), 3))
                    assert changed > 200, f'texture edit was not visible: {changed} changed pixels'
                    assert latency < 1.0, f'texture edit took {latency:.3f}s to reach the sampled frame'
                    for name in ('before', 'after', 'wave', 'model_reload', 'idle'):
                        shutil.copyfile(base / f'screenshots/{name}.tga', args.output / f'{name}.tga')
                    wave_image = Image.open(base / 'screenshots/wave.tga').convert('RGB').crop((18, 335, 621, 460))
                    reloaded = Image.open(base / 'screenshots/model_reload.tga').convert('RGB').crop((18, 335, 621, 460))
                    changed_pose = ImageChops.difference(wave_image, reloaded).tobytes()
                    assert sum(max(changed_pose[i:i + 3]) > 40 for i in range(0, len(changed_pose), 3)) > 30
                    (args.output / 'latency.txt').write_text(f'{latency:.6f}s source edit to rendered screenshot; {changed} changed preview pixels\n')
                finally:
                    if process.poll() is None:
                        process.kill()
                        process.wait()
                    input_device.close()
        finally:
            watcher.terminate()
            watcher.wait(timeout=10)
print('PASS: cooked Blender character in game; watched texture edit visible within one second')
