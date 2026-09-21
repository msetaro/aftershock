#!/usr/bin/env python3
"""Check native diagnostic routing and explicit formatter capacity with small text."""
import argparse
from pathlib import Path
from run import SCRATCH
import shlex
import subprocess

from run import ENV, ROOT, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-native-diagnostics'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
for module, cases in (
    ('game', ('print', 'error', 'shared-print', 'shared-error', 'log')),
    ('cgame', ('print', 'error', 'shared-print', 'shared-error')),
    ('ui', ('print', 'error')),
    ('bot', ('print',)),
):
    binary = args.output.resolve() / module
    run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
         '-include', 'game/bg/native_abi_public.h', '-DPROBE_' + module.upper(),
         '-ffunction-sections', '-fdata-sections', '-fsanitize=address,undefined',
         '-fno-sanitize-recover=all', 'tests/probes/native_diagnostics.cpp',
         '-Wl,--gc-sections', '-o', binary])
    for case in cases:
        result = subprocess.run([binary, case], cwd=ROOT, env=ENV,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if result.returncode or result.stdout:
            raise SystemExit(f'FAIL: {module} {case} diagnostic contract: '
                             f'exit {result.returncode}\n{result.stdout}')
    print('PASS:', module, 'diagnostic routing and formatter capacity')
