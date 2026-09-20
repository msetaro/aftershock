#!/usr/bin/env python3
"""Check development renderer snapshots and allocator accounting without a GPU."""
import argparse
from pathlib import Path
import shlex
from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-devtools-data'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
for probe in ('assets', 'memory'):
    binary = args.output.resolve() / probe
    run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
         '-Wall', '-Wextra', '-Werror', '-DAFTERSHOCK_DEVTOOLS', '-DUSE_VULKAN_API',
         '-ffunction-sections', '-fdata-sections', f'tests/probes/dev_{probe}.cpp',
         'engine/qcommon/q_shared.cpp', '-Wl,--gc-sections', '-o', binary])
    run([binary], timeout=10)
print('PASS: registry bounds/copies, bounded GPU timing copy and tagged/hunk accounting')
