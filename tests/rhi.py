#!/usr/bin/env python3
"""Check the GPU-free RHI stub and production frame upload contract."""
import argparse
from pathlib import Path
import shlex

from run import run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-rhi-tests'))
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    flags = [*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti',
             '-O2', '-Wall', '-Wextra', '-Werror', '-ffunction-sections', '-fdata-sections']
    for probe in ('stub', 'upload', 'image'):
        binary = args.output / probe
        run([*flags, '-DRHI_STUB_CHECK', '-DUSE_VULKAN_API',
             f'tests/probes/rhi_{probe}.cpp',
             *(['engine/qcommon/q_shared.cpp'] if probe == 'upload' else []),
             '-Wl,--gc-sections', '-o', binary])
        run([binary], timeout=10)
    # Non-system dependencies of the alternative backend must be the public API alone.
    deps = run([*shlex.split(args.cxx), '-std=c++20', '-MM',
                'tests/probes/rhi_stub.cpp'], capture_output=True, text=True).stdout
    paths = deps.replace('\\\n', ' ').split()[1:]
    if set(paths) != {'tests/probes/rhi_stub.cpp', 'tests/probes/../../engine/rhi/rhi_public.h'}:
        raise SystemExit('FAIL: RHI stub acquired a private dependency: ' + deps)
    for header in ('engine/renderervk/tr_local.h', 'engine/renderercommon/tr_public.h',
                   'engine/client/client_public.h'):
        deps = run([*shlex.split(args.cxx), '-std=c++20', '-DUSE_VULKAN_API', '-MM',
                    '-x', 'c++', header], capture_output=True, text=True).stdout
        if 'third_party/vulkan/' in deps or '/vk.h' in deps:
            raise SystemExit('FAIL: frontend header acquired a GPU SDK dependency: ' + deps)
    print('PASS: alternative backend links against only the public RHI header and reports unavailable')


if __name__ == '__main__':
    main()
