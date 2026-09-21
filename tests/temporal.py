#!/usr/bin/env python3
"""Check bounded previous-rendered-view/entity history without game simulation."""
import argparse
from pathlib import Path
import shlex

from run import ROOT, SCRATCH, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-temporal')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
implementation = ROOT/'engine/render/tr_temporal.cpp'
sources = [implementation] if implementation.exists() else []
probe = args.output/'probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/temporal.cpp', *sources, '-o', probe])
run([probe])
skin = args.output/'skin'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
     '-ffunction-sections', '-fdata-sections', 'tests/probes/temporal_skin.cpp', '-Wl,--gc-sections', '-o', skin])
run([skin])
print('PASS: bounded rendered-frame history, copied poses, camera cuts, identity, failed-frame rejection and previous skin vertices')
