#!/usr/bin/env python3
"""Reject out-of-bounds team-leader writes in the imported GPL C game sources."""
import argparse
from pathlib import Path

from run import run
from gpl_source import source_headers

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, default=Path('/tmp/aftershock-q3-gpl'))
args = parser.parse_args()
headers = source_headers(args.source)
for name in ('ai_cmd.c', 'ai_team.c'):
    run(['clang', '-std=gnu99', '-O2', '-Werror=array-bounds', '-fsyntax-only', '-DCOM_TRAP_GETVALUE=700',
         '-I' + str(headers), 'code/game/' + name])
print('PASS: both team-leader writes stay within the real GPL bot-state bounds')
