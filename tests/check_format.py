#!/usr/bin/env python3
"""Check owned C/C++ formatting with the reviewed clang-format version."""
import argparse
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
VERSION = '21.1.8'

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--clang-format', default='clang-format')
args = parser.parse_args()
tool = shlex.split(args.clang_format)
version = subprocess.check_output([*tool, '--version'], text=True)
if ('version', VERSION) not in zip(version.split(), version.split()[1:]):
    raise SystemExit('clang-format ' + VERSION + ' is required')
files = subprocess.check_output(
    ['git', 'ls-files', '-z', '--', 'engine', 'game', 'tools/port', 'tests/probes'],
    cwd=ROOT).decode().split('\0')
files = [name for name in files if Path(name).suffix in ('.h', '.cpp', '.c', '.inc')
         and not name.startswith('engine/platform/asm/')
         and name != 'engine/renderervk/shaders/spirv/shader_data.cpp']
if not files:
    raise SystemExit('no owned C/C++ files found')
subprocess.run([*tool, '--dry-run', '--Werror', *files], cwd=ROOT, check=True)
print('PASS:', len(files), 'owned files match clang-format', VERSION)
