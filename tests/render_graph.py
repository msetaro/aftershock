#!/usr/bin/env python3
"""Check graph lifetimes/dependencies and unchanged native target/pass descriptors."""
import argparse
import hashlib
from pathlib import Path
import shlex
from run import ROOT, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-render-graph'))
args = parser.parse_args()
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
run(['python3', 'tools/shaders/build.py', '--output', output / 'shaders'])
flags = [*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
         '-Wall', '-Wextra', '-Werror', '-ffunction-sections', '-fdata-sections']
# Keep the native observation runnable on the pre-graph revision for test-first evidence.
implementation = ROOT / 'engine/rhi/rhi_graph.cpp'
sources = [implementation] if implementation.is_file() else []
for name in ('native', 'contract'):
    binary = output / name
    if name == 'native':
        inputs = ['-DUSE_VULKAN_API', '-I' + str(output / 'shaders'),
                  'tests/probes/rhi_graph_native.cpp', 'engine/qcommon/q_shared.cpp',
                  'engine/qcommon/q_math.cpp']
    else:
        inputs = ['tests/probes/rhi_graph.cpp']
    run([*flags, *inputs, *sources, '-Wl,--gc-sections', '-o', binary])
    result = run([binary], capture_output=True, timeout=10)
    (output / (name + '.txt')).write_bytes(result.stdout)
    if name == 'native':
        # Initial reference at #7 merge e4f2d70a, matching GCC and Clang/libc++.
        digest = hashlib.sha256(result.stdout).hexdigest()
        assert digest == '962a3b48d9c358bde23fe52e9cb15dd9688b8c8efc083e363a2500c4680f3ddb', digest
        print('PASS: 36 native target/pass/framebuffer configurations match the reference', flush=True)
print('PASS: graph dependencies, retained/exported resources, lifetimes and fixed capacities')
