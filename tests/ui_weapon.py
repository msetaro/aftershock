#!/usr/bin/env python3
"""Check the native UI's negative weapon sentinel against its real state type."""
import argparse
from pathlib import Path
import shlex

from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='clang++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-ui-weapon'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve() / 'check'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-fsanitize=undefined', '-fno-sanitize-recover=all', '-DCOM_TRAP_GETVALUE=700',
     'tests/probes/ui_weapon.cpp', '-o', binary])
run([binary, '-1'])
print('PASS: native UI preserves the negative weapon sentinel and valid weapons')
