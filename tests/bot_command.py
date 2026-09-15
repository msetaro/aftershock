#!/usr/bin/env python3
"""Check bot movement conversion preserves command bytes without float UB."""
import argparse
from pathlib import Path
import shlex

from gpl_source import source_headers
from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='clang')
parser.add_argument('--source', type=Path, default=Path('/tmp/aftershock-q3-gpl'))
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-bot-command'))
args = parser.parse_args()
headers = source_headers(args.source)
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve() / 'check'
run([*shlex.split(args.cc), '-std=gnu99', '-O2', '-ffp-contract=off',
     '-ffunction-sections', '-fdata-sections', '-fsanitize=undefined,float-cast-overflow',
     '-fno-sanitize-recover=all', '-DCOM_TRAP_GETVALUE=700',
     '-Icode/game', '-I' + str(headers), 'code/game/ai_main.c',
     'tests/probes/bot_command.c', '-Wl,--gc-sections', '-lm', '-o', binary])
run([binary])
print('PASS: bot command bytes preserve truncation and wrapping')
