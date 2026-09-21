#!/usr/bin/env python3
"""Check CPU-affinity expressions without changing process affinity."""
import argparse
from pathlib import Path
from run import SCRATCH
import shlex

from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-affinity'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve() / 'check'
run([*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti', '-O2',
     '-ffunction-sections', '-fdata-sections', '-fsanitize=address,undefined',
     '-fno-sanitize-recover=all', 'tests/probes/affinity.cpp',
     '-Wl,--gc-sections', '-lm', '-o', binary])
run([binary])
print('PASS: affinity expressions and public apply path; OS affinity call intercepted')
