#!/usr/bin/env python3
"""Check real snapshot selection without changing the delta codec."""
import argparse
from pathlib import Path
import shlex

from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-replication-policy'))
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output / 'probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-fno-fast-math', '-ffp-contract=off', '-fno-strict-aliasing',
     '-ffunction-sections', '-fdata-sections', '-Wall', '-Wextra', '-Werror',
     '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/replication_policy.cpp', 'engine/qcommon/msg.cpp',
     'engine/qcommon/huffman.cpp', 'engine/qcommon/huffman_static.cpp',
     'engine/qcommon/q_shared.cpp', '-Wl,--gc-sections', '-o', binary])
run([binary])
