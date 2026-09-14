#!/usr/bin/env python3
"""Real client prediction, one process, impaired in-memory packets, no game sockets."""
import argparse
import json
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from run import ROOT, ENV, build, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-loopback-tests'))
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--max-error', type=float, default=64)
args = parser.parse_args()
output = args.output.resolve(); output.mkdir(parents=True, exist_ok=True)
if not args.max_error >= 0: parser.error('invalid error bound')
objects = output / 'build'
binary = build(objects, ['BUILD_SERVER=0', 'USE_RENDERER_DLOPEN=0', 'RENDERER_DEFAULT=vulkan']) / 'quake3e.x64'
wrapper = output / 'loopback.o'
run(['g++', '-std=c++20', '-fno-exceptions', '-fno-rtti', '-O2', '-c', 'tests/probes/loopback.cpp', '-o', wrapper])
# Reuse the exact production link command, adding only the test boundary wrappers.
recipe = run(['make', '-Bn', 'V=1', f'BUILD_DIR={objects}', 'BUILD_SERVER=0', 'USE_RENDERER_DLOPEN=0', 'RENDERER_DEFAULT=vulkan'], stdout=subprocess.PIPE).stdout.decode()
commands = [shlex.split(line) for line in recipe.splitlines() if ' -o ' + str(binary) + ' ' in line]
if len(commands) != 1: raise SystemExit('FAIL: expected one production link command')
command = commands[0]
wrapped = output / 'quake3e-loopback.x64'
command[command.index('-o')+1] = str(wrapped)
run([*command, wrapper, '-Wl,--wrap=_Z17NET_GetLoopPacket8netsrc_tP8netadr_tP5msg_t',
     '-Wl,--wrap=_Z9Com_Frame8qboolean', '-Wl,--wrap=_Z8NET_Initv'])
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if len(icds) != 1: parser.error('one Mesa lavapipe ICD required')
ENV.update(LIBGL_ALWAYS_SOFTWARE='1', GALLIUM_DRIVER='llvmpipe', LP_NUM_THREADS='1', VK_ICD_FILENAMES=str(icds[0]))
with tempfile.TemporaryDirectory(prefix='aftershock-loopback-') as temporary:
    home = Path(temporary); base = home / 'baseq3'; base.mkdir()
    for pak in args.data.resolve().glob('pak*.pk3'): (base / pak.name).symlink_to(pak)
    if not (base / 'pak0.pk3').is_file(): parser.error('user-owned baseq3 paks required')
    command = ['timeout', '90', 'xvfb-run', '-a', 'faketime', '-f', '@2026-01-01 00:00:00 i0.01', wrapped,
               '+set', 'fs_basepath', home, '+set', 'fs_homepath', home, '+set', 'net_enabled', '0',
               '+set', 'r_fullscreen', '0', '+set', 'r_mode', '3', '+set', 's_initsound', '0', '+set', 'sv_pure', '0',
               '+set', 'com_logfile', '0', '+set', 'com_maxfps', '0', '+set', 'fixedtime', '50',
               '+set', 'cg_showmiss', '1', '+set', 'name', 'regression', '+map', 'q3dm7',
               '+wait', '100', '+cg_showmiss', '+forward', '+wait', '180', '+-forward',
               '+wait', '120', '+test_loopback_report', '+quit']
    log = run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout.decode(errors='replace')
    (output/'client.log').write_text(log)
errors = [float(x) for x in re.findall(r'Prediction miss:\s*(\S+)', log)]
match = re.search(r'TEST_LOOPBACK received=(\d+),(\d+) delivered=(\d+),(\d+) dropped=(\d+) reordered=(\d+)', log)
if not match or 'regression^7 entered the game' not in log or 'cg_showmiss' not in log:
    raise SystemExit('FAIL: incomplete loopback/diagnostic coverage')
counts = [int(x) for x in match.groups()]
if not all(counts): raise SystemExit('FAIL: delay/loss/reordering must all be exercised')
result = dict(received=counts[:2], delivered=counts[2:4], dropped=counts[4], reordered=counts[5],
              errors=errors, maximum=max(errors, default=0), bound=args.max_error,
              loss_percent=5, delay_ms=[20,140], seed=1, transport='single-process memory loopback')
(output/'results.json').write_text(json.dumps(result, indent=2)+'\n')
if any(not 0 <= error <= args.max_error for error in errors): raise SystemExit('FAIL: prediction bound exceeded')
print('PASS: in-process prediction corrections within bound', result)
