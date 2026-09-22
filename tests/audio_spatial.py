#!/usr/bin/env python3
"""Exercise authored audio spatial math independently of device timing."""
import argparse
from pathlib import Path
import shlex

from run import SCRATCH, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=SCRATCH / 'aftershock-audio-spatial')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output / 'probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined',
     '-fno-sanitize-recover=all', 'tests/probes/audio_spatial.cpp',
     'engine/sound/snd_spatial.cpp', '-o', binary])
run([binary])
