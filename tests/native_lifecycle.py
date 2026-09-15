#!/usr/bin/env python3
"""Compare fresh and persistent native module storage across restart/map changes."""
import argparse
import difflib
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile

from native import build_modules, normalize_log
from run import ROOT, ENV, build, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--debug-movement', action='store_true', help='also compare movement debug counters')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-native-lifecycle'))
args = parser.parse_args()
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
if not paks:
    parser.error('installed Quake 3 content is required')
module = build_modules(output / 'modules', modules=('game',), language='c++')['game']
binary = build(output / 'engine', ['BUILD_CLIENT=0']) / 'quake3e.ded.x64'
shim = output / 'retain-modules.so'
run(['cc', '-Wall', '-Wextra', '-Werror', '-shared', '-fPIC',
     'tests/probes/retain_modules.c', '-o', shim])
results = {}
for mode in ('fresh', 'persistent-1', 'persistent-2'):
    with tempfile.TemporaryDirectory(prefix='aftershock-native-lifecycle-') as home:
        base = Path(home) / 'baseq3'
        base.mkdir()
        for pak in paks:
            (base / pak.name).symlink_to(pak)
        (base / module.name).symlink_to(module)
        command = ['timeout', '90', 'faketime', '-f', '@2026-01-01 00:00:00 i0.01', binary,
                   '+set', 'fs_basepath', home, '+set', 'fs_homepath', home,
                   '+set', 'vm_game', '0', '+set', 'dedicated', '1',
                   '+set', 'sv_pure', '0', '+set', 'g_debugMove', str(int(args.debug_movement)), '+set', 'com_logfile', '0',
                   '+map', 'q3dm17', '+addbot', 'sarge', '3', '+addbot', 'major', '3',
                   '+wait', '150', '+map_restart', '0', '+wait', '150',
                   '+map', 'q3dm7', '+addbot', 'sarge', '3', '+addbot', 'major', '3',
                   '+wait', '150', '+quit']
        env = ENV if mode == 'fresh' else {**ENV, 'LD_PRELOAD': str(shim)}
        result = subprocess.run([str(x) for x in command], cwd=ROOT, env=env,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (output / (mode + '.log')).write_bytes(result.stdout)
        result.check_returncode()
        if result.stdout.count(b'VM_LoadDll(qagame) succeeded!') != 3:
            raise SystemExit('FAIL: expected native game initialization, restart and map change')
        normalized = normalize_log(result.stdout).replace(home.encode(), b'<HOME>')
        normalized = re.sub(rb'^\.\.\.found [0-9]+ cached paks\r?\n|^Working directory:.*\r?\n',
                            b'', normalized, flags=re.M)
        results[mode] = normalized
        (output / (mode + '.normalized')).write_bytes(normalized)
        if normalized != results['fresh']:
            (output / (mode + '.diff')).write_text(''.join(difflib.unified_diff(
                results['fresh'].decode().splitlines(True), normalized.decode().splitlines(True),
                fromfile='fresh module storage', tofile=mode)))
            raise SystemExit('FAIL: module storage survives reload; see ' + str(output / (mode + '.diff')))
print('PASS: persistent module restart/map change', hashlib.sha256(results['fresh']).hexdigest())
