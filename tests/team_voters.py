#!/usr/bin/env python3
"""Check team voter counts without overwriting the adjacent spawn state."""
import argparse
from pathlib import Path
import shlex

from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='clang')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-team-voters'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve() / 'check'
# Keep the real rank calculation; isolate its unrelated end-level notifications.
flags = [*shlex.split(args.cc), '-std=gnu99', '-O2', '-fno-inline', '-fPIC',
         '-ffunction-sections', '-fdata-sections', '-fsanitize=undefined',
         '-fno-sanitize-recover=all', '-DCOM_TRAP_GETVALUE=700', '-Icode/game']
obj = args.output.resolve() / 'g_main.o'
run([*flags, '-c', 'code/game/g_main.c', '-o', obj])
run(['objcopy', '--weaken-symbol=CheckExitRules',
     '--weaken-symbol=SendScoreboardMessageToAllClients', obj])
run([*flags, obj, 'tests/probes/team_voters.c', '-Wl,--gc-sections',
     '-lm', '-o', binary])
run([binary])
print('PASS: team voter reset preserves adjacent state and excludes bots')
