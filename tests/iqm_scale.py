#!/usr/bin/env python3
"""Check production IQM joint scale/rotation and inverse composition."""
import argparse
from pathlib import Path
import shlex

from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-iqm-scale'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve() / 'check'
run([*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti', '-O2',
     '-DUSE_VULKAN_API', '-ffunction-sections', '-fdata-sections',
     '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/iqm_scale.cpp', '-Wl,--gc-sections', '-lm', '-o', binary])
run([binary])
