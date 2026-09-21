#!/usr/bin/env python3
"""Check UI skill conversion and score selection with finite cvar values."""
import argparse
from pathlib import Path
from run import SCRATCH
import shlex

from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-ui-skill'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve() / 'check'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-DCOM_TRAP_GETVALUE=700', '-ffunction-sections', '-fdata-sections',
     '-fsanitize=address,undefined,float-cast-overflow', '-fno-sanitize-recover=all',
     'tests/probes/ui_skill.cpp', 'game/ui/ui_gameinfo.cpp', 'game/bg/q_shared.cpp',
     '-Wl,--gc-sections', '-lm', '-o', binary])
run([binary, 'event'])
run([binary, 'score'])
print('PASS: UI skill selection and score storage preserve range policies')
