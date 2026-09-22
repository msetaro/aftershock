#!/usr/bin/env python3
"""Verify versioned POD fields and explicit N-to-N+1 state migration."""
import argparse
from pathlib import Path
import shlex
import sys
from run import ROOT, SCRATCH, run

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-state')
parser.add_argument('--cc',default='gcc')
parser.add_argument('--cxx',default='g++')
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
run([sys.executable, 'tools/replication.py', '--check'])
sha=args.output/'sha.o'
probe=args.output/'probe'
run([*shlex.split(args.cc),'-std=c99','-O2','-c','third_party/sha256/sha-256.c','-o',sha])
for definitions in ([], ['-DSTATE_NATIVE_GAME']):
    run([*shlex.split(args.cxx),*definitions,'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-Wconversion','-Wshadow','-fsanitize=undefined','-fno-sanitize-recover=all',
         'tests/probes/state.cpp','engine/qcommon/state.cpp',sha,'-o',probe])
    run([probe])
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
     '-fno-fast-math','-ffp-contract=off','-fno-strict-aliasing','-fwrapv','-fno-builtin',
     '-U_GNU_SOURCE','-D_DEFAULT_SOURCE','-D__NO_INLINE__','-Wall','-Werror',
     '-ffunction-sections','-fdata-sections','-fsanitize=undefined','-fno-sanitize-recover=all',
     'tests/probes/state_random.cpp','engine/qcommon/state.cpp',sha,'-Wl,--gc-sections','-o',probe])
run([probe])
