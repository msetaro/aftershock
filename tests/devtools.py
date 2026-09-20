#!/usr/bin/env python3
"""Check shipping exclusion and real ImGui cvar editing on a private Xvfb display."""
import argparse
import ctypes
import ctypes.util
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from run import ROOT, build, content_maps, content_settings
from window import wait_for
from native import engine_objects
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-devtools-tests'))
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--binary', type=Path, help='test an existing development client')
parser.add_argument('--inside-xvfb', action='store_true', help=argparse.SUPPRESS)
args = parser.parse_args()
if not args.binary:
    objects = engine_objects(args.output.resolve() / 'native', args.content, 'gcc', 'g++')
    for enabled in (False, True):
        directory = build(args.output.resolve() / ('enabled' if enabled else 'shipping'), ['BUILD_SERVER=0', f'AFTERSHOCK_DEVTOOLS={int(enabled)}', *objects])
        binary = directory / 'quake3e.x64'
        symbols = subprocess.check_output(['nm', '-C', '--defined-only', binary], text=True)
        (directory / 'symbols.txt').write_text(symbols)
        for required in ('ImGui::NewFrame()', 'DevTools_Draw('):
            if (required in symbols) != enabled:
                raise SystemExit(f'FAIL: {required} must be present only with AFTERSHOCK_DEVTOOLS=ON')
        print('PASS:', 'development tooling linked' if enabled else 'shipping binary excludes tooling', flush=True)
    args.binary = binary
if not args.inside_xvfb:
    subprocess.run(['timeout', '90', 'xvfb-run', '-a', sys.executable, str(Path(__file__).resolve()), '--inside-xvfb', '--binary', str(args.binary.resolve()), '--output', str(args.output.resolve()), '--content', args.content, '--data', str(args.data.resolve())], cwd=ROOT, check=True)
    raise SystemExit(0)
root = ROOT
out = args.output.resolve() / 'runtime'
out.mkdir(parents=True, exist_ok=True)
binary = args.binary.resolve()
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if not binary.is_file() or not paks or len(icds) != 1:
    parser.error('an existing client, installed content and one lavapipe ICD are required')
if not ctypes.util.find_library('X11') or not ctypes.util.find_library('Xtst'):
    parser.error('libX11 and libXtst are required (CI installs libxtst6)')
map_name = content_maps(args.content)[0]
golden = ROOT / 'tests/golden' / ('openarena' if args.content == 'openarena' else '')
env = dict(os.environ, LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
x11 = ctypes.CDLL(ctypes.util.find_library('X11'))
xt = ctypes.CDLL(ctypes.util.find_library('Xtst'))
x11.XOpenDisplay.argtypes = [ctypes.c_char_p]
x11.XOpenDisplay.restype = ctypes.c_void_p
x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
x11.XSync.argtypes = [ctypes.c_void_p, ctypes.c_int]
x11.XStringToKeysym.argtypes = [ctypes.c_char_p]
x11.XStringToKeysym.restype = ctypes.c_ulong
x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
x11.XKeysymToKeycode.restype = ctypes.c_ubyte
xt.XTestFakeRelativeMotionEvent.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_ulong]
xt.XTestFakeButtonEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
xt.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_int, ctypes.c_ulong]
display = x11.XOpenDisplay(None)
assert display
cursor = [320, 240]

def click(x, y):
    xt.XTestFakeRelativeMotionEvent(display, x - cursor[0], y - cursor[1], 0)
    x11.XSync(display, 0)
    time.sleep(0.15)
    xt.XTestFakeButtonEvent(display, 1, 1, 0)
    x11.XSync(display, 0)
    time.sleep(0.1)
    xt.XTestFakeButtonEvent(display, 1, 0, 0)
    x11.XSync(display, 0)
    time.sleep(0.2)
    cursor[:] = [x, y]

def key(name):
    code = x11.XKeysymToKeycode(display, x11.XStringToKeysym(name.encode()))
    assert code
    xt.XTestFakeKeyEvent(display, code, 1, 0)
    x11.XSync(display, 0)
    time.sleep(0.06)
    xt.XTestFakeKeyEvent(display, code, 0, 0)
    x11.XSync(display, 0)
    time.sleep(0.06)
with tempfile.TemporaryDirectory(prefix='aftershock-dev-input-') as temp:
    home = Path(temp)
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    (base / 'demos').mkdir(parents=True)
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    shutil.copyfile(golden / (map_name + '.dm_68'), base / 'demos' / (map_name + '.dm_68'))
    command = [str(binary), '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home),
               *content_settings(args.content), '+set', 'r_fullscreen', '0', '+set', 'r_mode', '3',
               '+set', 's_initsound', '0', '+set', 'net_enabled', '0', '+set', 'sv_pure', '0',
               '+set', 'cl_autoRecordDemo', '0', '+set', 'dev_tools', '1', '+set', 'devtest', '0',
               '+set', 'com_maxfps', '20', '+exec', 'devtools-check.cfg']
    script = [f'demo {map_name}', 'wait 140', 'screenshot cvars', 'devtools_status',
              'wait 80', 'devtools_status', 'devtest', 'vid_restart', 'wait 40',
              'screenshot restarted', 'wait 2', 'devtools_status']
    for tab in ('textures', 'materials', 'profile', 'memory'):
        script += ['echo inspect_' + tab, 'wait 40', 'screenshot ' + tab, 'wait 2']
    script += ['quit']
    (base / 'devtools-check.cfg').write_text('\n'.join(script) + '\n')
    with (out / 'client.log').open('wb') as log:
        process = subprocess.Popen(command, cwd=root, env=env, stdout=log, stderr=subprocess.STDOUT)
        try:
            wait_for(lambda: b'CL_InitCGame:' in (out / 'client.log').read_bytes(), process)
            tree = subprocess.check_output(['xwininfo', '-root', '-tree'], text=True)
            candidates = re.findall('^\\s*(0x[0-9a-fA-F]+).*640x480', tree, re.M)
            owned = [window for window in candidates if re.search(
                r'=\s*' + str(process.pid) + r'\s*$',
                subprocess.check_output(['xprop', '-id', window, '_NET_WM_PID'], text=True))]
            assert len(owned) == 1, 'expected one window owned by the launched client'
            time.sleep(0.5)
            click(100, 64)
            click(100, 91)
            for char in 'devtest':
                key(char)
            click(100, 118)
            click(100, 383)
            key('Home')
            key('Delete')
            key('7')
            key('Return')
            for tab, x in (('textures', 160), ('materials', 240), ('profile', 310), ('memory', 375)):
                wait_for(lambda: ('inspect_' + tab).encode() in (out / 'client.log').read_bytes(), process)
                # Context recreation retains our relative pointer position.
                click(x, 64)
            process.wait(timeout=30)
            assert process.returncode == 0
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
    for name in ('cvars', 'restarted', 'textures', 'materials', 'profile', 'memory'):
        image = base / 'screenshots' / (name + '.tga')
        assert struct.unpack_from('<HH', image.read_bytes(), 12) == (640, 480)
        shutil.copyfile(image, out / image.name)
x11.XCloseDisplay(display)
text = (out / 'client.log').read_text()
assert '"devtest" is:"7^7"' in text, 'live ImGui cvar edit failed; see ' + str(out / 'client.log')
assert not any(error in text for error in ('ERROR:', 'Signal caught', 'capacity exceeded', 'Unknown command'))
samples = [tuple(map(int, row)) for row in re.findall('Developer tools: enabled=1 frames=(\\d+) arena=(\\d+)/16777216 allocations=(\\d+)', text)]
assert len(samples) == 3 and samples[1][0] - samples[0][0] >= 60
assert samples[0][2] == samples[1][2], 'idle UI made allocations after warmup'
assert all(0 < sample[1] < 16777216 for sample in samples)
assert samples[2][0] > samples[1][0] and text.count('Static ui loaded.') >= 3
profiles = re.findall(r'Developer profile: cpu=(\d+) snapshots=(\d+) bits=(\d+)', text)
assert len(profiles) == 3 and all(all(int(value) > 0 for value in row) for row in profiles)
print('PASS: real mouse/key cvar edit, bounded UI memory, allocation-free idle frames and renderer restart')
