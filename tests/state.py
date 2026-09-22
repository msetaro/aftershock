#!/usr/bin/env python3
"""Verify versioned POD fields and explicit N-to-N+1 state migration."""
import argparse
from pathlib import Path
import shlex
import sys
import re
import subprocess
from check_boundaries import TOKENS, blank
from run import ROOT, SCRATCH, run

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-state')
parser.add_argument('--cc',default='gcc')
parser.add_argument('--cxx',default='g++')
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
# Keep every owned engine libc draw visible to checkpoint capture.
raw_random=re.compile(r'\b(?:rand|srand)\s*\(')
assert raw_random.search('rand()') and raw_random.search('std::srand(1)')
assert not raw_random.search(TOKENS.sub(lambda match: blank(match[0]), '// rand()\n"srand()"'))
for name in subprocess.check_output(['git','ls-files','-z','--','engine'],cwd=ROOT).decode().split('\0'):
    if Path(name).suffix not in ('.cpp','.h','.c','.inc') or name in (
            'engine/qcommon/q_shared.cpp','engine/renderervk/shaders/spirv/shader_data.cpp'):
        continue
    source=TOKENS.sub(lambda match: blank(match[0]), (ROOT/name).read_text())
    assert not raw_random.search(source), f'{name}: use Q_Rand/Q_Srand so checkpoints retain the stream'
# Every entity callback assignment needs a stable identity in its owner table.
assigned, registered = set(), set()
callback_kinds = r'(think|reached|blocked|touch|use|pain|die)'
for path in (ROOT/'game/game').glob('*.cpp'):
    source=path.read_text()
    code=TOKENS.sub(lambda match: blank(match[0]), source)
    for kind, value in re.findall(r'(?:\b\w+|\])(?:->|\.)'+callback_kinds+r'\s*=(?!=)\s*([^;]+);', code):
        value=value.strip()
        if value in ('0','NULL','nullptr'):
            continue
        assert re.fullmatch(r'\w+', value), f'{path}: describe indirect callback assignment: {value}'
        assigned.add((kind,value))
    for name, fields in re.findall(r'\{\s*\.name\s*=\s*"(\w+)"\s*,([^}]+)\}',source):
        for kind, value in re.findall(r'\.'+callback_kinds+r'\s*=\s*(\w+)',fields):
            assert name==value, f'{path}: callback identity must retain its stable function name'
            assert (kind,value) not in registered, f'duplicate callback identity: {name}'
            registered.add((kind,value))
assert assigned==registered, f'callback coverage: missing {assigned-registered}, unused {registered-assigned}'
print(f'PASS: {len(assigned)} assigned entity callbacks have typed, stable save identities')
run([sys.executable, 'tools/replication.py', '--check'])
sha=args.output/'sha.o'
probe=args.output/'probe'
shared=args.output/'shared.o'
run([*shlex.split(args.cc),'-std=c99','-O2','-c','third_party/sha256/sha-256.c','-o',sha])
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections','-fsanitize=undefined',
     '-fno-sanitize-recover=all','-c','engine/qcommon/q_shared.cpp','-o',shared])
for definitions in ([], ['-DSTATE_NATIVE_GAME']):
    run([*shlex.split(args.cxx),*definitions,'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-Wconversion','-Wshadow','-fsanitize=undefined','-fno-sanitize-recover=all',
         '-ffunction-sections','-fdata-sections',
         'tests/probes/state.cpp','engine/qcommon/state.cpp',shared,sha,'-Wl,--gc-sections','-o',probe])
    run([probe])
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-fno-fast-math','-ffp-contract=off','-fno-strict-aliasing','-fwrapv','-fno-builtin',
     '-U_GNU_SOURCE','-D_DEFAULT_SOURCE','-D__NO_INLINE__','-Wall','-Werror',
     '-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all',
     'tests/probes/state_random.cpp','engine/qcommon/state.cpp',sha,'-Wl,--gc-sections','-o',probe])
run([probe])

run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
     '-fsanitize=undefined','-fno-sanitize-recover=all',
     'tests/probes/state_callbacks.cpp','engine/qcommon/state.cpp',sha,'-Wl,--gc-sections','-o',probe])
run([probe])
