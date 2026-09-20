#!/usr/bin/env python3
"""Preserve the pre-#12 entity/player delta bytes and check generated descriptions."""
import argparse
import hashlib
from pathlib import Path
import shlex
import runpy
import subprocess
import sys

from run import ROOT, ENV, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-replication'))
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output / 'probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-fno-strict-aliasing', '-ffunction-sections', '-fdata-sections',
     '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/replication.cpp', 'engine/qcommon/huffman.cpp',
     'engine/qcommon/huffman_static.cpp', 'engine/qcommon/q_shared.cpp',
     '-Wl,--gc-sections', '-o', binary])
wire = subprocess.check_output([binary], cwd=ROOT, env=ENV)
(args.output / 'wire.bin').write_bytes(wire)
# Captured from e1ff877f with GCC and Clang/libc++ before generated tables.
expected = '26a5fc0d8e5afbfcc1634ddbfcf67155c6a2c6156066b86d20088690dff7d496'
assert hashlib.sha256(wire).hexdigest() == expected, 'field order/layout or delta bytes changed'
print('PASS: 256 entity/player delta round trips, removal and pre-#12 wire digest', flush=True)
run([sys.executable, 'tools/replication.py', '--check'])

# Source edits must not silently omit a member or reuse a wire position.
generator = runpy.run_path(str(ROOT / 'tools/replication.py'))
source = (ROOT / 'engine/qcommon/q_shared.h').read_text()
for before, after in [
    ('// @net 34:8', '// missing annotation'),
    ('// @net 34:8', '// @net 0:8'),
    ('// @net [0]:1:0 [1]:2:0 [2]:9:0', '// @net [0]:1:0 [1]:2:0'),
    ('// @net 34:8', '// @net 34:33'),
    ('// @net 34:8', '// @net [0]:34:8'),
]:
    assert before in source
    try:
        generator['generate'](source.replace(before, after, 1))
    except ValueError:
        pass
    else:
        raise AssertionError('generator accepted invalid source annotation: ' + after)
print('PASS: annotation coverage, unique order, valid member paths and widths')
