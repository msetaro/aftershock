#!/usr/bin/env python3
"""Exercise native calls with every supported argument count in the real engine."""
import argparse
from pathlib import Path

from run import build, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='cc')
parser.add_argument('--cxx', default='c++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-native-dispatch'))
args = parser.parse_args()
variables = [f'CC={args.cc}', f'CXX={args.cxx}', 'BUILD_CLIENT=0', 'USE_SDL=0', 'USE_CURL=0']
variables += ['LDFLAGS=tests/probes/native_dispatch.cpp -Wl,--wrap=main -std=c++20 -fno-exceptions -fno-rtti -lm -ldl']
# The probe is a link input supplied here, outside the Makefile's dependencies.
(args.output.resolve() / 'build/release-linux-x86_64/quake3e.ded.x64').unlink(missing_ok=True)
binary = build(args.output.resolve() / 'build', variables) / 'quake3e.ded.x64'
run([binary])
