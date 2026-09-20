#!/usr/bin/env python3
"""Compare current C, C++ game and engine wire/module layouts."""
import argparse
from pathlib import Path
import shlex
import subprocess

from run import ROOT, ENV

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='gcc')
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-native-abi'))
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
layouts = []
for name, compiler, mode in [
    ('c', args.cc, ['-x', 'c', '-std=gnu99', '-include', 'game/bg/native_abi_public.h']),
    ('cpp', args.cxx, ['-x', 'c++', '-std=c++20', '-include', 'game/bg/native_abi_public.h']),
    ('engine', args.cxx, ['-x', 'c++', '-std=c++20', '-DENGINE'])
]:
    binary = args.output / name
    command = [*shlex.split(compiler), *mode, 'tests/probes/native_layout.c', '-o', str(binary)]
    (args.output / (name + '.command')).write_text(shlex.join(command) + '\n')
    subprocess.run(command, cwd=ROOT, env=ENV, check=True)
    layouts.append(subprocess.check_output([binary], env=ENV))
    (args.output / (name + '.txt')).write_bytes(layouts[-1])
if len(layouts[0].splitlines()) != 33 or len(set(layouts)) != 1:
    raise SystemExit('FAIL: native/engine ABI layouts differ; see ' + str(args.output))
print('PASS: 29 wire/module types, three offsets and the service extension agree in C/game C++/engine C++')
