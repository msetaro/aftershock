#!/usr/bin/env python3
"""Check download URL construction without performing a transfer."""
import argparse
from pathlib import Path
import shlex
import subprocess

from run import build, compare, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-download-tests'))
    parser.add_argument('--regenerate', action='store_true')
    args = parser.parse_args()
    args.output = args.output.resolve()
    objects = args.output / 'build/release-linux-x86_64/client'
    sources = ['cl_curl', 'q_shared']
    build(args.output / 'build', [f'CC={args.cc}', f'CXX={args.cxx}',
          'BUILD_CLIENT=1', 'BUILD_SERVER=0', 'USE_SDL=0', 'USE_CURL=1',
          'USE_CURL_DLOPEN=0', 'CFLAGS=-ffunction-sections -fdata-sections'],
          [objects / (name + '.o') for name in sources])
    binary = args.output / 'download'
    run([*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti',
         '-DUSE_CURL', '-ffunction-sections', '-fdata-sections', 'tests/probes/download.cpp',
         *[objects / (name + '.o') for name in sources], '-Wl,--gc-sections',
         '-lcurl', '-o', binary])
    actual = run([binary], stdout=subprocess.PIPE).stdout
    compare('download.txt', actual, args.regenerate)


if __name__ == '__main__':
    main()
