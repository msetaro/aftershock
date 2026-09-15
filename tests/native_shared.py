#!/usr/bin/env python3
"""Compare native C/C++ math words and case conversion on the same host."""
import argparse
from pathlib import Path
import shlex
import subprocess

from run import ROOT, ENV

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='gcc')
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-native-shared'))
args = parser.parse_args()
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
results = []
for language, compiler in [('c', args.cc), ('c++', args.cxx)]:
    compiler = shlex.split(compiler)
    mode = ['-std=gnu99'] if language == 'c' else ['-std=c++20', '-fno-exceptions', '-fno-rtti', '-U_GNU_SOURCE', '-D_DEFAULT_SOURCE']
    binary = output / language.replace('+', 'p')
    command = [*compiler, '-x', language, *mode, '-O2', '-DNDEBUG', '-fno-builtin',
               '-ffp-contract=off', '-fno-strict-aliasing', '-fwrapv',
               '-include', 'game/bg/native_abi.h', '-ffunction-sections', '-fdata-sections',
               'game/bg/q_math.cpp', 'game/bg/q_shared.cpp', 'tests/probes/native_shared.c',
               '-Wl,--gc-sections', '-lm', '-o', str(binary)]
    (output / (binary.name + '.command')).write_text(shlex.join(command) + '\n')
    with (output / (binary.name + '.log')).open('w') as log:
        subprocess.run(command, cwd=ROOT, env=ENV, stdout=log, stderr=subprocess.STDOUT, check=True)
    results.append(subprocess.check_output([binary], env=ENV))
    (output / (binary.name + '.txt')).write_bytes(results[-1])
if results[0] != results[1]:
    raise SystemExit('FAIL: native C/C++ shared functions differ; see c.txt and cpp.txt')
print('PASS: C/C++ shared functions agree\n' + results[0].decode(), end='')
