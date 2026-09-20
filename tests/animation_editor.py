#!/usr/bin/env python3
"""Author a graph in ImGui, cook its edit, and preview the changed state."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time

from run import ROOT, build, content_maps, content_settings
from window import XInput, wait_for

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path)
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-animation-editor'))
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--inside-xvfb', action='store_true', help=argparse.SUPPRESS)
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
if not args.binary:
    args.binary = build(args.output / 'build', ['BUILD_SERVER=0', 'AFTERSHOCK_DEVTOOLS=1']) / 'quake3e.x64'
if not args.inside_xvfb:
    subprocess.run(['timeout', '90', 'xvfb-run', '-a', sys.executable, __file__, '--inside-xvfb',
                    '--binary', str(args.binary.resolve()), '--output', str(args.output),
                    '--content', args.content, '--data', str(args.data.resolve())], cwd=ROOT, check=True)
    raise SystemExit(0)
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if not paks or len(icds) != 1:
    parser.error('installed content and one lavapipe ICD are required')
with tempfile.TemporaryDirectory(prefix='aftershock-animation-editor-') as temporary:
    home = Path(temporary)
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    source = base / 'animation_source'
    shutil.copytree(ROOT / 'tests/assets/animation', source)
    graph = source / 'rifle.animation.json'
    document = json.loads(graph.read_text())
    document = {'initial_state': document.pop('initial_state'), **document}
    graph.write_text(json.dumps(document, indent=2) + '\n')
    original = graph.read_bytes()
    (base / 'editor.cfg').write_text('\n'.join([
        f'devmap {content_maps(args.content)[0]}', 'wait 10',
        'dev_animation load animations/anim_rifle.asanim',
        'dev_animation source animation_source/rifle.animation.json',
        'set dev_tools 1', 'echo animation_editor_ready', 'wait 400',
        'dev_animation load animations/anim_rifle.asanim', 'wait 30',
        'devtools_status', 'screenshot animation_editor', 'wait 3', 'quit']) + '\n')
    log_path = args.output / 'client.log'
    with (args.output / 'watch.log').open('wb') as watch_log:
        watcher = subprocess.Popen([sys.executable, 'tools/cook', str(source / 'rigs.json'), '--output', str(base), '--watch'],
                                   cwd=ROOT, stdout=watch_log, stderr=subprocess.STDOUT)
        process = None
        device = None
        try:
            wait_for(lambda: (base / 'cook.revision').is_file(), watcher)
            revision = (base / 'cook.revision').read_bytes()
            with log_path.open('wb') as log:
                process = subprocess.Popen([str(args.binary.resolve()), '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home),
                    *content_settings(args.content), '+set', 'net_enabled', '0', '+set', 'sv_pure', '0',
                    '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0', '+set', 's_initsound', '0',
                    '+set', 'con_notifytime', '0', '+set', 'com_maxfps', '20', '+set', 'cl_autoRecordDemo', '0', '+exec', 'editor.cfg'],
                    cwd=ROOT, env=dict(os.environ, LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0])),
                    stdout=log, stderr=subprocess.STDOUT)
                wait_for(lambda: b'Animation graph loaded: state=idle' in log_path.read_bytes(), process, 15)
                device = XInput()
                device.verify_window(process)
                # Graph tab is selected by the load command; edit its source tab.
                device.click(120, 115)
                device.click(150, 230)
                device.key_event('Control_L', True)
                device.key('Home')
                device.key_event('Control_L', False)
                device.key('Down')
                device.key('Home')
                for _ in range(len('  "initial_state": "')):
                    device.key('Right')
                device.key_event('Shift_L', True)
                for _ in range(4):
                    device.key('Right')
                device.key_event('Shift_L', False)
                for key in 'ads':
                    device.key(key)
                device.click(180, 165)
                wait_for(lambda: graph.read_bytes() != original, process)
                assert json.loads(graph.read_text())['initial_state'] == 'ads'
                backups = list(source.glob('rifle.animation.json.bak.*'))
                assert len(backups) == 1 and backups[0].read_bytes() == original
                wait_for(lambda: (base / 'cook.revision').read_bytes() != revision, watcher)
                device.click(45, 115)
                process.wait(timeout=40)
                assert process.returncode == 0
            text = log_path.read_text()
            assert 'Animation graph loaded: state=ads' in text
            assert re.search(r'Developer graph: state=ads previews=[1-9]\d*', text)
            assert not any(error in text for error in ('ERROR:', 'Signal caught', 'Animation rejected'))
            shutil.copyfile(base / 'screenshots/animation_editor.tga', args.output / 'animation_editor.tga')
        finally:
            if device:
                device.close()
            for child in (process, watcher):
                if child and child.poll() is None:
                    child.terminate()
                    try:
                        child.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        child.kill()
                        child.wait(timeout=5)
print('PASS: ImGui source edit, retained backup, watched cook and changed graph preview')
