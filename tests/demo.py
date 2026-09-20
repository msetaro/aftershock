#!/usr/bin/env python3
"""Replay fixed .dm_68 fixtures and compare Mesa software timedemo frame goldens."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

from run import ROOT, ENV, build, run, content_maps, content_bots, content_settings
from frames import check_frames
from native import engine_objects, verify_static

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-demo-tests'))
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--cc', default='gcc')
parser.add_argument('--cxx', default='g++')
parser.add_argument('--lifecycle', action='store_true', help='check fixed frames after replay and video restart')
parser.add_argument('--measure-gpu', action='store_true', help='after the frame gate, measure Vulkan scopes with the real clock')
parser.add_argument('--pipeline-cache', action='store_true', help='verify cache restoration across fresh client processes')
parser.add_argument('--modules', action='store_true', help='exercise the optional PC renderer module boundary')
parser.add_argument('--record-fixtures', action='store_true', help='explicitly replace demos and frame goldens')
parser.add_argument('--regenerate', action='store_true', help='explicitly replace frame goldens only')
args = parser.parse_args()
if args.lifecycle and (args.record_fixtures or args.regenerate):
    parser.error('lifecycle comparison requires fixed fixtures')
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
objects = engine_objects(output / 'native', args.content, args.cc, args.cxx)
binaries = {}
for backend in ('vulkan',):
    directory = build(output / ('build-' + backend), [f'CC={args.cc}', f'CXX={args.cxx}', *objects,
                      'BUILD_SERVER=0', f'USE_RENDERER_DLOPEN={int(args.modules)}',
                      'RENDERER_DEFAULT=' + backend])
    binaries[backend] = directory / 'quake3e.x64'
    verify_static(binaries[backend], ('game', 'cgame', 'ui'))
    symbols = subprocess.check_output(['nm', '-C', '--defined-only', binaries[backend]], text=True)
    if any(' GetRefAPI(' in line for line in symbols.splitlines()) == args.modules:
        raise SystemExit('FAIL: renderer linkage does not match --modules')


def client(binary, home, commands, log_name, fixed_random=False, real_clock=False, require_cache=False):
    # Loading a fixture is restricted to this offline invocation, never Xvfb itself.
    preload = ['env', 'LD_PRELOAD=' + str(shim)] if fixed_random else []
    clock = [] if real_clock else ['faketime', '-f', '@2026-01-01 00:00:00 i0.01']
    command = ['timeout', '90', 'xvfb-run', '-a', *preload, *clock, binary,
               '+set', 'fs_basepath', home, '+set', 'fs_homepath', home,
               *content_settings(args.content),
               '+set', 'r_fullscreen', '0', '+set', 'r_mode', '3', '+set', 's_initsound', '0',
               '+set', 'sv_pure', '0', '+set', 'net_ip', '127.0.0.1', '+set', 'com_maxfps', '0',
               '+set', 'com_logfile', '0', '+set', 'cl_autoRecordDemo', '0', '+set', 'name', 'regression', *commands]
    started = time.monotonic()
    result = subprocess.run([str(a) for a in command], cwd=ROOT, env=ENV, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    print(f'{log_name}: {time.monotonic() - started:.3f}s wall time (includes startup)', flush=True)
    log = result.stdout
    (output / log_name).write_bytes(log)
    result.check_returncode()
    if any(f'Static {name} loaded.'.encode() not in log for name in ('cgame', 'ui')):
        raise SystemExit('FAIL: static UI/cgame were not initialized')
    if args.lifecycle and not real_clock and any(log.count(f'Static {name} loaded.'.encode()) < 2 for name in ('cgame', 'ui')):
        raise SystemExit('FAIL: native modules were not restarted: ' + log_name)
    if (require_cache or args.lifecycle and not real_clock) and binary == binaries['vulkan'] and b'pipeline cache: loaded ' not in log:
        raise SystemExit('FAIL: renderer restart did not restore its pipeline cache: ' + log_name)
    if b'Unknown command' in log or b'ERROR:' in log:
        raise SystemExit('FAIL: client reported an error: ' + log_name)
    marker = b'VK_RENDERER:'
    if marker not in log:
        raise SystemExit('FAIL: wrong renderer: ' + log_name)
    if b'llvmpipe' not in log.lower():
        raise SystemExit('FAIL: renderer did not report the forced software device: ' + log_name)
    if real_clock:
        timings = [line for line in log.decode().splitlines() if line.startswith('gpu ')]
        if not timings:
            raise SystemExit('FAIL: real-clock GPU timing results unavailable: ' + log_name)
        print('\n'.join(timings), flush=True)


def prepare(home):
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    if cache_home:
        (base / 'cache').symlink_to(cache_home.name, target_is_directory=True)
    # Reuse installed content without reading the user's loose configs or copying paks.
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    (base / 'demos').mkdir()
    return base


cache_home = tempfile.TemporaryDirectory(prefix='aftershock-pipeline-cache-') if args.pipeline_cache else None

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
                        '+wait', '100', '+screenshot', 'frame200', '+wait', '2', '+gfxinfo',
                        *(['+vkinfo'] if backend == 'vulkan' else []), '+quit'],
                       f'{map_name}-{backend}-replay-{iteration}.log',
                       require_cache=args.pipeline_cache and iteration == 2)
                for name in ('frame050', 'frame100', 'frame200'):
                    shutil.copyfile(base / 'screenshots' / (name + '.tga'),
                                    output / f'{map_name}-{backend}-{iteration}-{name}.tga')
check_frames(output, args.content, args.regenerate)

if args.measure_gpu:
    # Software drivers also read the clock. Never treat faketime query values as
    # performance measurements; the accepted screenshot comparison ran above.
    for map_name in content_maps(args.content):
        for iteration in (1, 2):
            with tempfile.TemporaryDirectory(prefix='aftershock-timing-') as temporary:
                home = Path(temporary)
                base = prepare(home)
                fixture = golden / (map_name + '.dm_68')
                shutil.copyfile(fixture, base / 'demos' / fixture.name)
                client(binaries['vulkan'], home,
                       ['+set', 'timedemo', '1', '+demo', map_name, '+wait', '200', '+gfxinfo', '+vkinfo', '+quit'],
                       f'{map_name}-vulkan-timing-{iteration}.log', real_clock=True)
