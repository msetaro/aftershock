#!/usr/bin/env python3
"""Check the imported game math word width against the original 32-bit routine."""
import argparse
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='cc')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-native-math'))
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
for mode, flags in [('release', ['-O2']), ('asan', ['-O0', '-fsanitize=address'])]:
    binary = args.output / mode
    command = [*shlex.split(args.cc), '-x', 'c', '-std=c99', *flags, '-fno-strict-aliasing',
               '-ffp-contract=off', '-ffunction-sections', '-fdata-sections',
               'game/bg/q_math.cpp', 'tests/probes/native_math.c', '-Wl,--gc-sections',
               '-lm', '-o', str(binary)]
    with (args.output / (mode + '.log')).open('w') as log:
        subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
        subprocess.run([binary], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
    print('PASS:', mode)
