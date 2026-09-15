#!/usr/bin/env python3
"""Check OpenArena's string helpers using its pinned public source."""
import argparse
from pathlib import Path
import shlex
import subprocess

from run import ROOT, run

REVISION = '331464ca396d80e91cf9be273588f2b5f4b7afc8'
REPOSITORY = 'https://github.com/OpenArena/gamecode'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='gcc')
parser.add_argument('--source', type=Path, default=Path('/tmp/aftershock-oa-native-source'))
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-openarena-strings'))
args = parser.parse_args()
source, output = args.source.resolve(), args.output.resolve()
if not source.exists():
    run(['git', 'init', source])
    run(['git', '-C', source, 'fetch', '--depth=1', REPOSITORY, REVISION])
for name in ('q_shared.c', 'q_shared.h', 'q_platform.h', 'surfaceflags.h'):
    path = Path('code/qcommon') / name
    target = output / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(subprocess.check_output(['git', '-C', source, 'show', REVISION + ':' + str(path)]))
for name in ('openarena-name-comparison.patch', 'openarena-extension.patch'):
    patch = ROOT / 'tests/patches' / name
    if patch.exists():
        subprocess.run(['git', 'apply', str(patch)], cwd=output, check=True)
binary = output / 'check'
run([*shlex.split(args.cc), '-std=gnu99', '-O2', '-fsanitize=undefined',
     '-fno-sanitize-recover=all', '-I' + str(output / 'code/qcommon'),
     'tests/probes/openarena_strings.c', '-o', binary])
run([binary])
print('PASS: OpenArena name comparison handles absent names and evaluates arguments once')
binary = output / 'extension'
run([*shlex.split(args.cc), '-std=gnu99', '-O2', '-fno-builtin',
     '-ffunction-sections', '-fdata-sections', '-fsanitize=address',
     '-I' + str(output / 'code/qcommon'), output / 'code/qcommon/q_shared.c',
     'tests/probes/openarena_extension.c', '-Wl,--gc-sections', '-lm', '-o', binary])
run([binary])
print('PASS: OpenArena extension stripping preserves in-place and bounded paths')
