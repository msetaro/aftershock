#!/usr/bin/env python3
"""Check every movement-result field on the production engine's early return."""
import argparse
from pathlib import Path
import shlex
import subprocess

from run import build, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='cc')
parser.add_argument('--cxx', default='c++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-bot-move'))
args = parser.parse_args()
args.output = args.output.resolve()
variables = [f'CC={args.cc}', f'CXX={args.cxx}', 'BUILD_CLIENT=0', 'USE_SDL=0', 'USE_CURL=0']
directory = args.output / 'build'
server = build(directory, variables) / 'quake3e.ded.x64'
recipe = run(['make', '-Bn', 'V=1', f'BUILD_DIR={directory}', *variables],
             stdout=subprocess.PIPE).stdout.decode()
commands = [shlex.split(line) for line in recipe.splitlines() if ' -o ' in line and ' -c ' not in line]
command = next(c for c in commands if c[c.index('-o') + 1] == str(server))
binary = args.output / 'bot-move'
command[command.index('-o') + 1] = str(binary)
run([*command, '-std=c++20', '-fno-exceptions', '-fno-rtti',
     'tests/probes/bot_move.cpp', '-Wl,--wrap=main'])
run([binary])
