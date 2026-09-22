#!/usr/bin/env python3
"""Exercise the native backend client through platform/HTTP seams without any live account."""
import argparse
from pathlib import Path
import shlex
from run import run,SCRATCH
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx',default='g++')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-backend-client')
args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
binary=args.output/'client'
run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti','-fsanitize=undefined',
     '-fno-sanitize-recover=all','-ffunction-sections','-fdata-sections','tests/probes/backend_client.cpp',
     'engine/qcommon/q_shared.cpp','engine/qcommon/json.cpp','-Wl,--gc-sections','-o',binary])
run([binary])
