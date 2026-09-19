#!/usr/bin/env python3
"""Check team-message formatter results with small text and an injected return value."""
import argparse
from pathlib import Path
import shlex
import subprocess

from run import ENV, ROOT, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-team-message'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
for mode, flags in [('base', []), ('missionpack', ['-DMISSIONPACK'])]:
    binary = args.output.resolve() / mode
    run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
         '-DCOM_TRAP_GETVALUE=700', *flags, '-ffunction-sections', '-fdata-sections',
         '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
         'tests/probes/team_message.cpp', '-Wl,--gc-sections', '-o', binary])
    for case, expected in [('valid', 0), ('fit', 0), ('full', 42), ('error', 42)]:
        result = subprocess.run([binary, case], cwd=ROOT, env=ENV,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if result.returncode != expected or result.stdout:
            raise SystemExit(f'FAIL: {mode} formatter result {case}: expected exit {expected}, '
                             f'got {result.returncode}\n{result.stdout}')
    print('PASS:', mode, 'team-message formatting and result policy')
