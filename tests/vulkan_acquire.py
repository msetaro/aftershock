#!/usr/bin/env python3
"""Check frame acquisition result handling without a GPU or window."""
import argparse
from pathlib import Path
import shlex
import subprocess

from run import run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-vulkan-acquire-tests'))
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    binary = args.output / 'acquire'
    run([*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti',
         '-O2', '-DNDEBUG', '-Wall', '-Wextra', '-Werror', '-DUSE_VULKAN_API',
         '-ffunction-sections', '-fdata-sections', 'tests/probes/vulkan_acquire.cpp',
         'engine/qcommon/q_shared.cpp', 'engine/qcommon/q_math.cpp',
         '-Wl,--gc-sections', '-lm', '-o', binary])
    failures = []
    # VkResult ABI values: success/suboptimal acquired an image; timeout/not-ready did not.
    for name, result, expected in [('success', 0, 0), ('suboptimal', 1000001003, 0),
                                   ('timeout', 2, 42), ('not-ready', 1, 42)]:
        outcome = subprocess.run([binary, str(result)], capture_output=True, text=True, timeout=10)
        log = outcome.stdout + outcome.stderr
        (args.output / (name + '.log')).write_text(log)
        if outcome.returncode != expected:
            failures.append(f'{name}: exit {outcome.returncode}, expected {expected}: {log.strip()}')
    if failures:
        raise SystemExit('\n'.join(failures))
    print('PASS: valid/suboptimal acquisition records commands; timeout/not-ready reports an error before acquisition')


if __name__ == '__main__':
    main()
