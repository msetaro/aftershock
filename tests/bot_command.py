#!/usr/bin/env python3
"""Check bot movement conversion preserves command bytes without float UB."""
import argparse
from pathlib import Path
import shlex

from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='clang')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-bot-command'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve() / 'check'
run([*shlex.split(args.cc), '-x', 'c', '-std=gnu99', '-O2', '-ffp-contract=off',
     '-ffunction-sections', '-fdata-sections', '-fsanitize=undefined,float-cast-overflow',
     '-fno-sanitize-recover=all', '-include', 'game/bg/native_abi_public.h',
     '-Igame/game', 'game/game/ai_main.cpp',
     'tests/probes/bot_command.c', '-Wl,--gc-sections', '-lm', '-o', binary])
run([binary])
print('PASS: bot command bytes preserve truncation and wrapping')
