#!/usr/bin/env python3
"""Native pure-list markers retain data-pak checksum accounting."""
import argparse
from pathlib import Path
import shlex
from run import run
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx',default='g++')
parser.add_argument('--output',type=Path,default=Path('/tmp/aftershock-native-pure'))
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
binary=args.output.resolve()/'pure'
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti','-fno-strict-aliasing',
     '-ffunction-sections','-fdata-sections','tests/probes/native_pure.cpp','engine/qcommon/q_shared.cpp',
     '-Wl,--gc-sections','-o',binary])
run([binary])
print('PASS: native pure markers and referenced content-pak checksum accounting')
