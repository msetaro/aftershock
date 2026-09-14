#!/usr/bin/env python3
"""Reject out-of-bounds team-leader writes in the imported GPL C game sources."""
import argparse
from pathlib import Path
import subprocess

from run import run

REVISION = 'dbe4ddb10315479fc00086f08e25d968b4b43c49'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=Path, default=Path('/tmp/aftershock-q3-gpl'))
args = parser.parse_args()
source = args.source.resolve()
if not source.exists():
    run(['git', 'clone', '--no-checkout', 'https://github.com/id-Software/Quake-III-Arena', source])
    run(['git', '-C', source, 'checkout', '--detach', REVISION])
revision = run(['git', '-C', source, 'rev-parse', 'HEAD'], stdout=subprocess.PIPE).stdout.decode().strip()
if revision != REVISION:
    raise SystemExit('FAIL: unexpected GPL header revision: ' + revision)
run(['git', '-C', source, 'diff', '--quiet', REVISION, '--', 'code/game/*.h', 'code/botlib/*.h', 'code/qcommon/*.h'])
for name in ('ai_cmd.c', 'ai_team.c'):
    run(['clang', '-std=gnu99', '-O2', '-Werror=array-bounds', '-fsyntax-only',
         '-I' + str(source / 'code/game'), 'code/game/' + name])
print('PASS: both team-leader writes stay within the real GPL bot-state bounds')
