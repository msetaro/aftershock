#!/usr/bin/env python3
"""Exercise null and installed platform services through the same bounded interface."""
import argparse
from pathlib import Path
import shlex
from run import SCRATCH, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-services')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve()/'services'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/services.cpp', 'engine/platform/sys_services.cpp', '-o', binary])
run([binary])
