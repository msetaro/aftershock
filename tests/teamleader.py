#!/usr/bin/env python3
"""Reject out-of-bounds team-leader writes in the imported GPL C game sources."""
from run import run

for name in ('ai_cmd.c', 'ai_team.c'):
    run(['clang', '-std=gnu99', '-O2', '-Werror=array-bounds', '-fsyntax-only',
         '-include', 'code/game/native_abi.h',
         'code/game/' + name])
print('PASS: both team-leader writes stay within the real GPL bot-state bounds')
