#!/usr/bin/env python3
"""Check reusable temporary PCM storage without reopening files at loop boundaries."""
import argparse
from pathlib import Path
import shlex

from run import SCRATCH, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=SCRATCH / 'aftershock-audio-streams')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output / 'probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-ffunction-sections', '-fdata-sections', '-fsanitize=undefined',
     '-fno-sanitize-recover=all', 'tests/probes/audio_streams.cpp',
     'engine/platform/unix/unix_shared.cpp', '-Wl,--gc-sections', '-o', binary])
run([binary, args.output / 'owned-temporary-pcm'])
