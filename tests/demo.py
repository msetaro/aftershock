#!/usr/bin/env python3
"""Offline deterministic .dm_68 recording and Mesa software timedemo frame goldens."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from run import ROOT, ENV, build, compare, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-demo-tests'))
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--regenerate', action='store_true')
args = parser.parse_args()
if args.regenerate and os.environ.get('CI'):
    parser.error('CI must never regenerate goldens')
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
data = args.data.resolve()
paks = sorted(data.glob('pak*.pk3'))
if not (data / 'pak0.pk3').is_file():
    parser.error('user-owned baseq3 paks are required; see tests/README.md')
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if len(icds) != 1:
    parser.error('exactly one installed Mesa lavapipe ICD is required')
ENV.update(LIBGL_ALWAYS_SOFTWARE='1', GALLIUM_DRIVER='llvmpipe', LP_NUM_THREADS='1',
           VK_ICD_FILENAMES=str(icds[0]), VK_DRIVER_FILES=str(icds[0]))
shim = output / 'fixed-random.so'
run(['cc', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC', 'tests/probes/fixed_random.c', '-ldl', '-o', shim])
binaries = {}
for backend in ('vulkan', 'opengl1'):
    directory = build(output / ('build-' + backend), ['BUILD_SERVER=0', 'USE_RENDERER_DLOPEN=0', 'RENDERER_DEFAULT=' + backend])
    binaries[backend] = directory / 'quake3e.x64'


def client(binary, home, commands, log_name, fixed_random=False):
    # Loading a fixture is restricted to this offline invocation, never Xvfb itself.
    preload = ['env', 'LD_PRELOAD=' + str(shim)] if fixed_random else []
    command = ['timeout', '90', 'xvfb-run', '-a', *preload, 'faketime', '-f', '@2026-01-01 00:00:00 i0.01', binary,
               '+set', 'fs_basepath', home, '+set', 'fs_homepath', home,
               '+set', 'r_fullscreen', '0', '+set', 'r_mode', '3', '+set', 's_initsound', '0',
               '+set', 'sv_pure', '0', '+set', 'net_ip', '127.0.0.1', '+set', 'com_maxfps', '0',
               '+set', 'com_logfile', '0', '+set', 'cl_autoRecordDemo', '0', '+set', 'name', 'regression', *commands]
    log = run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout
    (output / log_name).write_bytes(log)
    if b'llvmpipe' not in log.lower():
        raise SystemExit('FAIL: renderer did not report the forced software device: ' + log_name)


def prepare(home):
    base = home / 'baseq3'
    base.mkdir()
    # Reuse installed content without reading the user's loose configs or copying paks.
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    (base / 'demos').mkdir()
    return base


frames = {}
for map_name in ('q3dm17', 'q3dm7'):
    recordings = []
    for iteration in (1, 2):
        with tempfile.TemporaryDirectory(prefix='aftershock-record-') as temporary:
            home = Path(temporary)
            base = prepare(home)
            client(binaries['vulkan'], home,
                   ['+set', 'g_synchronousClients', '1', '+set', 'fixedtime', '50',
                    '+map', map_name, '+addbot', 'sarge', '3', '+addbot', 'major', '3',
                    '+wait', '100', '+record', map_name, '+wait', '300', '+stoprecord', '+quit'],
                   f'{map_name}-record-{iteration}.log', fixed_random=True)
            demo = base / 'demos' / (map_name + '.dm_68')
            recordings.append(demo.read_bytes())
    if recordings[0] != recordings[1]:
        for i, recording in enumerate(recordings):
            (output / f'{map_name}-different-{i}.dm_68').write_bytes(recording)
        raise SystemExit('FAIL: repeated demo bytes differ: ' + map_name)
    fixture = ROOT / 'tests/golden' / (map_name + '.dm_68')
    if args.regenerate:
        fixture.write_bytes(recordings[0])
    elif fixture.read_bytes() != recordings[0]:
        raise SystemExit('FAIL: recorded demo differs from golden: ' + map_name)
    print('PASS repeated demo', map_name, hashlib.sha256(recordings[0]).hexdigest(), flush=True)
    for backend, binary in binaries.items():
        repetitions = []
        for iteration in (1, 2):
            with tempfile.TemporaryDirectory(prefix='aftershock-replay-') as temporary:
                home = Path(temporary)
                base = prepare(home)
                shutil.copyfile(fixture, base / 'demos' / fixture.name)
                client(binary, home,
                       ['+set', 'timedemo', '1', '+demo', map_name,
                        '+wait', '50', '+screenshot', 'frame050',
                        '+wait', '50', '+screenshot', 'frame100',
                        '+wait', '100', '+screenshot', 'frame200', '+wait', '2', '+quit'],
                       f'{map_name}-{backend}-replay-{iteration}.log')
                hashes = {name: hashlib.sha256((base / 'screenshots' / (name + '.tga')).read_bytes()).hexdigest()
                          for name in ('frame050', 'frame100', 'frame200')}
                repetitions.append(hashes)
        if repetitions[0] != repetitions[1]:
            raise SystemExit('FAIL: repeated timedemo frames differ: ' + map_name + '/' + backend)
        frames[map_name + '/' + backend] = repetitions[0]
        print('PASS repeated frames', map_name, backend, flush=True)
compare('frames.json', (json.dumps(frames, indent=2, sort_keys=True) + '\n').encode(), args.regenerate)
