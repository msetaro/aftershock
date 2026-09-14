#!/usr/bin/env python3
"""Check every movement-result field on the production engine's early return."""
import argparse
from pathlib import Path

from run import build, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='cc')
parser.add_argument('--cxx', default='c++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-bot-move'))
args = parser.parse_args()
args.output = args.output.resolve()
variables = [f'CC={args.cc}', f'CXX={args.cxx}', 'BUILD_CLIENT=0', 'USE_SDL=0', 'USE_CURL=0']
variables += ['LDFLAGS=tests/probes/bot_move.cpp -Wl,--wrap=main -std=c++20 -fno-exceptions -fno-rtti -lm -ldl']
binary = build(args.output / 'build', variables) / 'quake3e.ded.x64'
run([binary])
