#!/usr/bin/env python3
"""Check download URLs and curl options without network access."""
import argparse
from pathlib import Path
import shlex
import subprocess

from run import build_objects, compare, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-download-tests'))
    parser.add_argument('--regenerate', action='store_true')
    args = parser.parse_args()
    args.output = args.output.resolve()
    sources = ['cl_curl', 'q_shared']
    objects = build_objects(args.output / 'build', [f'CC={args.cc}', f'CXX={args.cxx}',
          'BUILD_CLIENT=1', 'BUILD_SERVER=0', 'USE_SDL=0', 'USE_CURL=1',
          'USE_CURL_DLOPEN=0', 'CFLAGS=-ffunction-sections -fdata-sections'],
          'client', sources)
    binary = args.output / 'download'
    run([*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti',
         '-DUSE_CURL', '-ffunction-sections', '-fdata-sections', 'tests/probes/download.cpp',
         *[objects[name] for name in sources], '-Wl,--gc-sections',
         '-lcurl', '-o', binary])
    actual = run([binary], stdout=subprocess.PIPE).stdout
    compare('download.txt', actual, args.regenerate)
    options = args.output / 'curl-options'
    run([*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti',
         '-O2', '-fno-strict-aliasing', '-DUSE_CURL', '-Werror=varargs',
         '-ffunction-sections', '-fdata-sections', 'tests/probes/curl.cpp',
         '-Wl,--gc-sections', '-lcurl', '-o', options])
    fixture = args.output / 'curl-input.txt'
    fixture.write_text('aftershock curl options\n')
    run([options, fixture.as_uri()], timeout=10)


if __name__ == '__main__':
    main()
