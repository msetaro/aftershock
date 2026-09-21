#!/usr/bin/env python3
"""Check texture residency decisions against real allocation costs and peak VRAM."""
import argparse
from pathlib import Path
import shlex
from run import ROOT, SCRATCH, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-streaming')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
source = ROOT/'engine/render/tr_stream.cpp'
sources = [source] if source.exists() else []
probe = args.output/'policy'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/streaming.cpp', *sources, '-o', probe])
run([probe])
print('PASS: bounded texture residency, peak allocation accounting, eviction, fairness and frame wrap')
