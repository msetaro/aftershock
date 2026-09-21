#!/usr/bin/env python3
"""Validate bounded server hit-box history under deterministic latency/loss/jitter."""
import argparse
from pathlib import Path
from run import SCRATCH
import shlex

from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-rewind'))
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output / 'probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-fno-fast-math', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
     '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/rewind.cpp', 'engine/qcommon/net_history.cpp', '-o', binary])
run([binary])

game = args.output / 'game-probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-fno-fast-math', '-ffp-contract=off', '-Wall', '-Wextra', '-Werror',
     '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/rewind_game.cpp', 'engine/qcommon/net_history.cpp', '-o', game])
run([game])
