#!/usr/bin/env python3
"""Two separately compiled engine protocol versions must refuse each other."""
import argparse
from pathlib import Path
from run import SCRATCH
import shlex
import subprocess

from run import ROOT, ENV, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-protocol'))
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
probes, versions = [], []
for version in (1, 2):
    binary = args.output / f'protocol-{version}'
    run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
         '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections',
         '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
         f'-DAFTERSHOCK_NET_VERSION={version}', 'tests/probes/protocol.cpp',
         'engine/qcommon/msg.cpp', 'engine/qcommon/huffman.cpp',
         'engine/qcommon/huffman_static.cpp', 'engine/qcommon/q_shared.cpp',
         '-Wl,--gc-sections', '-o', binary])
    versions.append(subprocess.check_output([binary], cwd=ROOT, env=ENV, text=True).split())
    probes.append(binary)
assert versions[0][0] == '1' and versions[1][0] == '2'
assert versions[0][1] == versions[1][1] and len(versions[0][1]) == 64
for index, binary in enumerate(probes):
    run([binary, *versions[1 - index]])
print('PASS: independent protocol builds accept their schema/version and refuse mismatched peers')
