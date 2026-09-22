#!/usr/bin/env python3
"""Verify explicit SDL dummy capture and bounded callback buffering without a microphone."""
import argparse
from pathlib import Path
import shlex
import subprocess
from run import SCRATCH, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=SCRATCH / 'aftershock-capture')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'sdl2'], text=True))
binary = args.output / 'probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-ffunction-sections', '-fdata-sections', '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/capture.cpp', '-Wl,--gc-sections', *flags, '-o', binary])
run([binary], timeout=15)
