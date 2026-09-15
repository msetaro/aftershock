#!/usr/bin/env python3
"""Check bot chat's unmatched-variable sentinel under both char defaults."""
import argparse
from pathlib import Path
import shlex

from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-chat-offset'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
for sign in ('signed', 'unsigned'):
    binary = args.output.resolve() / sign
    run([*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti', '-O2',
         '-f' + sign + '-char', '-ffunction-sections', '-fdata-sections',
         '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
         'tests/probes/chat_offset.cpp', 'engine/qcommon/q_shared.cpp',
         '-Wl,--gc-sections', '-lm', '-o', binary])
    run([binary])
    print('PASS: bot chat extraction/expansion with ' + sign + ' char', flush=True)
