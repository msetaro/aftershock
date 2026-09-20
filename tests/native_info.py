#!/usr/bin/env python3
"""Check in-place native info-string removal with the real GPL helpers."""
import argparse
from pathlib import Path
import shlex

from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='clang')
parser.add_argument('--variant', choices=['small', 'big'])
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-native-info'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve() / 'check'
run([*shlex.split(args.cc), '-x', 'c', '-std=gnu99', '-O2', '-g',
     '-fsanitize=address', '-fno-omit-frame-pointer', '-fno-builtin',
     '-ffunction-sections', '-fdata-sections', '-Igame/bg',
     'game/bg/q_shared.cpp', 'tests/probes/native_info.c',
     '-Wl,--gc-sections', '-lm', '-o', binary])
for variant in ([args.variant] if args.variant else ['small', 'big']):
    run([binary, variant])
    print('PASS:', variant, 'info removal preserves the remaining pairs')
