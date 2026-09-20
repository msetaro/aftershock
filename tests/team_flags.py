#!/usr/bin/env python3
"""Check real team flag initialization and updates under UBSan."""
import argparse
from pathlib import Path
import shlex

from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='clang')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-team-flags'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
for mode, flags in [('base', []), ('missionpack', ['-DMISSIONPACK'])]:
    binary = args.output.resolve() / mode
    run([*shlex.split(args.cc), '-std=gnu99', '-O2', *flags,
         '-ffunction-sections', '-fdata-sections', '-fsanitize=undefined',
         '-fno-sanitize-recover=all', '-include', 'game/bg/native_abi_public.h',
         'tests/probes/team_flags.c', '-Wl,--gc-sections', '-lm', '-o', binary])
    run([binary])
    print('PASS:', mode, 'flag initialization and updates')
