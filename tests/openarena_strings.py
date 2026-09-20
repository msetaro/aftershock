#!/usr/bin/env python3
"""Check OpenArena's string helpers using its pinned public source."""
import argparse
from pathlib import Path
import shlex

from run import run
from openarena_native import stage_source

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='gcc')
parser.add_argument('--source', type=Path, default=Path('/tmp/aftershock-oa-native-source'))
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-openarena-strings'))
args = parser.parse_args()
output = stage_source(args.output, args.source)
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
