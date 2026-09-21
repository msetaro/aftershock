#!/usr/bin/env python3
"""Exercise the real identity lifecycle and platform discovery seam with a fake provider."""
import argparse
from pathlib import Path
from run import SCRATCH
import shlex
from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-identity'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve() / 'probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
     '-ffunction-sections', '-fdata-sections', 'tests/probes/identity.cpp',
     'engine/platform/sys_services.cpp', '-Wl,--gc-sections', '-o', binary])
run([binary])

ui = args.output.resolve() / 'discovery-ui'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-fsanitize=undefined',
     '-fno-sanitize-recover=all', '-ffunction-sections', '-fdata-sections',
     'tests/probes/discovery_ui.cpp', 'engine/qcommon/q_shared.cpp',
     '-Wl,--gc-sections', '-o', ui])
run([ui])
