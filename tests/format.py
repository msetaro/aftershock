#!/usr/bin/env python3
"""Check shared formatting capacity with the real engine and native helpers."""
import argparse
from pathlib import Path
import shlex
import subprocess

from run import ENV, ROOT, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='cc')
parser.add_argument('--cxx', default='c++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-format'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
for name, directory, compiler, mode, defines in (
    ('engine', 'engine/qcommon', args.cxx, 'c++', ['-DENGINE_PROBE']),
    ('game-c', 'game/bg', args.cc, 'c', []),
    ('game-cpp', 'game/bg', args.cxx, 'c++', []),
):
    binary = args.output.resolve() / name
    flags = ['-std=gnu99'] if mode == 'c' else ['-std=c++20', '-fno-exceptions', '-fno-rtti']
    run([*shlex.split(compiler), '-x', mode, *flags, *defines, '-O1', '-g',
         '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
         '-fno-omit-frame-pointer', '-ffunction-sections', '-fdata-sections',
         '-I' + directory, directory + '/q_shared.cpp', 'tests/probes/format.cpp',
         '-Wl,--gc-sections', '-lm', '-o', binary])
    for case in ('valid', 'va-valid'):
        run([binary, case])
    for case in ('overflow', 'va-overflow'):
        result = subprocess.run([binary, case], cwd=ROOT, env=ENV,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if result.returncode != 42 or result.stdout:
            raise SystemExit(f'FAIL: {name} {case} must reject oversized formatting before writing: '
                             f'exit {result.returncode}\n{result.stdout}')
    print('PASS:', name, 'shared formatting preserves valid text and rejects capacity overflow')
