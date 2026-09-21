#!/usr/bin/env python3
"""Compare static restart/map changes with the reviewed DLL-reload reference."""
import argparse
from pathlib import Path
from run import SCRATCH
import re
import subprocess
import tempfile

from native import normalize_log
from run import ROOT, ENV, build, compare

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--debug-movement', action='store_true', help='also compare movement debug counters')
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-native-lifecycle'))
args = parser.parse_args()
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
if not paks:
    parser.error('installed Quake 3 content is required')
binary = build(output / 'engine', ['BUILD_CLIENT=0']) / 'quake3e.ded.x64'
# Captured from ordinary module reloads with the d6c2ac52 game, before static integration.
golden = 'native-lifecycle-debug.log' if args.debug_movement else 'native-lifecycle.log'
for mode in ('1', '2'):
    with tempfile.TemporaryDirectory(prefix='aftershock-native-lifecycle-') as home:
        base = Path(home) / 'baseq3'
        base.mkdir()
        for pak in paks:
            (base / pak.name).symlink_to(pak)
        command = ['timeout', '90', 'faketime', '-f', '@2026-01-01 00:00:00 i0.01', binary,
                   '+set', 'fs_basepath', home, '+set', 'fs_homepath', home,
                   '+set', 'dedicated', '1',
                   '+set', 'sv_pure', '0', '+set', 'g_debugMove', str(int(args.debug_movement)), '+set', 'com_logfile', '0',
                   '+map', 'q3dm17', '+addbot', 'sarge', '3', '+addbot', 'major', '3',
                   '+wait', '150', '+map_restart', '0', '+wait', '150',
                   '+map', 'q3dm7', '+addbot', 'sarge', '3', '+addbot', 'major', '3',
                   '+wait', '150', '+quit']
        result = subprocess.run([str(x) for x in command], cwd=ROOT, env=ENV,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (output / (mode + '.log')).write_bytes(result.stdout)
        result.check_returncode()
        if (b'Static game loaded.' not in result.stdout or
                result.stdout.count(b'------- Game Initialization -------') != 3):
            raise SystemExit('FAIL: expected native game initialization, restart and map change')
        normalized = normalize_log(result.stdout).replace(home.encode(), b'<HOME>')
        normalized = re.sub(rb'^\.\.\.found [0-9]+ cached paks\r?\n|^Working directory:.*\r?\n',
                            b'', normalized, flags=re.M)
        (output / (mode + '.normalized')).write_bytes(normalized)
        compare(golden, normalized, False)
print('PASS: static restart/map change matches the DLL-reload reference twice')
