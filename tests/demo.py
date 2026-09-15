#!/usr/bin/env python3
"""Replay fixed .dm_68 fixtures and compare Mesa software timedemo frame goldens."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from run import ROOT, ENV, build, run, content_maps, content_bots, content_settings
from frames import check_frames

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-demo-tests'))
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--game-code', choices=['qvm', 'native'], default='qvm')
parser.add_argument('--game-language', choices=['c', 'c++'], default='c')
parser.add_argument('--cc', default='gcc')
parser.add_argument('--cxx', default='g++')
parser.add_argument('--lifecycle', action='store_true', help='compare fresh and retained modules after replay and video restart')
parser.add_argument('--record-fixtures', action='store_true', help='explicitly replace demos and frame goldens')
parser.add_argument('--regenerate', action='store_true', help='explicitly replace frame goldens only')
args = parser.parse_args()
if args.lifecycle and (args.game_code != 'native' or args.record_fixtures or args.regenerate):
    parser.error('lifecycle comparison requires native modules and fixed fixtures')
if args.game_code == 'native' and (args.record_fixtures or args.regenerate):
    parser.error('native parity requires fixed fixtures without regeneration')
if args.record_fixtures:
    args.regenerate = True
if args.regenerate and os.environ.get('CI'):
    parser.error('CI must never regenerate goldens')
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
data = args.data.resolve()
paks = sorted(data.glob('*.pk3'))
if not paks:
    parser.error('installed content paks are required; see tests/README.md')
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if len(icds) != 1:
    parser.error('exactly one installed Mesa lavapipe ICD is required')
ENV.update(LIBGL_ALWAYS_SOFTWARE='1', GALLIUM_DRIVER='llvmpipe', LP_NUM_THREADS='1',
           VK_ICD_FILENAMES=str(icds[0]), VK_DRIVER_FILES=str(icds[0]))
shim = output / 'fixed-random.so'
run(['cc', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC', 'tests/probes/fixed_random.c', '-ldl', '-o', shim])
retain_shim = output / 'retain-modules.so'
if args.lifecycle:
    run(['cc', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
         'tests/probes/retain_modules.c', '-o', retain_shim])
modules = {}
if args.game_code == 'native':
    from native import build_modules
    modules = build_modules(output / 'native', args.cc, ('cgame', 'ui'), args.cxx, args.game_language, args.content)
binaries = {}
for backend in ('vulkan', 'opengl1'):
    directory = build(output / ('build-' + backend), ['BUILD_SERVER=0', 'USE_RENDERER_DLOPEN=0', 'RENDERER_DEFAULT=' + ('opengl' if backend == 'opengl1' else backend)])
    binaries[backend] = directory / 'quake3e.x64'


def client(binary, home, commands, log_name, fixed_random=False, retain_modules=False):
    # Loading a fixture is restricted to this offline invocation, never Xvfb itself.
    preload = ['env', 'LD_PRELOAD=' + str(shim)] if fixed_random else []
    if retain_modules:
        preload = ['env', 'LD_PRELOAD=' + str(retain_shim)]
    command = ['timeout', '90', 'xvfb-run', '-a', *preload, 'faketime', '-f', '@2026-01-01 00:00:00 i0.01', binary,
               '+set', 'fs_basepath', home, '+set', 'fs_homepath', home,
               *content_settings(args.content),
               *(['+set', 'vm_cgame', '0', '+set', 'vm_ui', '0'] if modules else []),
               '+set', 'r_fullscreen', '0', '+set', 'r_mode', '3', '+set', 's_initsound', '0',
               '+set', 'sv_pure', '0', '+set', 'net_ip', '127.0.0.1', '+set', 'com_maxfps', '0',
               '+set', 'com_logfile', '0', '+set', 'cl_autoRecordDemo', '0', '+set', 'name', 'regression', *commands]
    result = subprocess.run([str(a) for a in command], cwd=ROOT, env=ENV, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log = result.stdout
    (output / log_name).write_bytes(log)
    result.check_returncode()
    if modules and (any(f'VM_LoadDll({name}) succeeded!'.encode() not in log for name in modules) or b'Failed to load dll' in log):
        raise SystemExit('FAIL: native UI/cgame were not loaded')
    if args.lifecycle and any(log.count(f'VM_LoadDll({name}) succeeded!'.encode()) < 2 for name in modules):
        raise SystemExit('FAIL: native modules were not restarted: ' + log_name)
    if b'Unknown command' in log or b'ERROR:' in log:
        raise SystemExit('FAIL: client reported an error: ' + log_name)
    marker = b'GL_RENDERER:' if binary == binaries['opengl1'] else b'VK_RENDERER:'
    if marker not in log:
        raise SystemExit('FAIL: wrong renderer: ' + log_name)
    if b'llvmpipe' not in log.lower():
        raise SystemExit('FAIL: renderer did not report the forced software device: ' + log_name)


def prepare(home):
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    # Reuse installed content without reading the user's loose configs or copying paks.
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    for module in modules.values():
        (base / module.name).symlink_to(module)
    (base / 'demos').mkdir()
    return base


golden = ROOT / 'tests/golden' / ('openarena' if args.content == 'openarena' else '')
for map_name in content_maps(args.content):
    fixture = golden / (map_name + '.dm_68')
    if args.record_fixtures:
        with tempfile.TemporaryDirectory(prefix='aftershock-record-') as temporary:
            home = Path(temporary)
            base = prepare(home)
            client(binaries['vulkan'], home,
                   ['+set', 'g_synchronousClients', '1', '+set', 'fixedtime', '50',
                    '+map', map_name, '+addbot', content_bots(args.content)[0], '3',
                    '+addbot', content_bots(args.content)[1], '3',
                    '+wait', '100', '+team', 'spectator', '+follow', '1', '+wait', '10', '+record', map_name, '+wait', '300', '+stoprecord', '+quit'],
                   f'{map_name}-record.log', fixed_random=True)
            fixture.write_bytes((base / 'demos' / fixture.name).read_bytes())
    print('FIXTURE', map_name, hashlib.sha256(fixture.read_bytes()).hexdigest(), flush=True)
    for backend, binary in binaries.items():
        for iteration in (1, 2):
            with tempfile.TemporaryDirectory(prefix='aftershock-replay-') as temporary:
                home = Path(temporary)
                base = prepare(home)
                shutil.copyfile(fixture, base / 'demos' / fixture.name)
                client(binary, home,
                       ['+set', 'timedemo', '1',
                        *(['+demo', map_name, '+wait', '200', '+disconnect', '+wait', '2',
                           '+vid_restart', '+wait', '2'] if args.lifecycle else []),
                        '+demo', map_name,
                        '+wait', '50', '+screenshot', 'frame050',
                        '+wait', '50', '+screenshot', 'frame100',
                        '+wait', '100', '+screenshot', 'frame200', '+wait', '2', '+quit'],
                       f'{map_name}-{backend}-replay-{iteration}.log',
                       retain_modules=args.lifecycle and iteration == 2)
                for name in ('frame050', 'frame100', 'frame200'):
                    shutil.copyfile(base / 'screenshots' / (name + '.tga'),
                                    output / f'{map_name}-{backend}-{iteration}-{name}.tga')
check_frames(output, args.content, args.regenerate, compare_goldens=not args.lifecycle)
