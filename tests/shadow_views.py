#!/usr/bin/env python3
"""Check shadow-view projection, culling and stable cascade coverage analytically."""
import argparse
from pathlib import Path
import shlex
from run import ROOT, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-shadow-views'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve() / 'check'
source = ROOT / 'engine/render/tr_shadow.cpp'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-Wall', '-Wextra', '-Werror', '-DUSE_VULKAN_API', '-ffp-contract=off',
     '-ffunction-sections', '-fdata-sections', '-fsanitize=undefined',
     '-fno-sanitize-recover=all', 'tests/probes/shadow_views.cpp',
     *([source] if source.is_file() else []), 'engine/qcommon/q_math.cpp',
     '-Wl,--gc-sections', '-o', binary])
run([binary])
scene = args.output.resolve() / 'scene'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-Wall', '-Wextra', '-Werror', '-DUSE_VULKAN_API', '-ffp-contract=off',
     '-ffunction-sections', '-fdata-sections', '-fsanitize=undefined',
     '-fno-sanitize-recover=all', 'tests/probes/scene_lights.cpp',
     'engine/qcommon/q_math.cpp', '-Wl,--gc-sections', '-o', scene])
run([scene])

raster = args.output.resolve() / 'raster'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-Wall', '-Wextra', '-Werror', '-DUSE_VULKAN_API', '-ffunction-sections',
     '-fdata-sections', '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/shadow_raster.cpp', '-Wl,--gc-sections', '-o', raster])
run([raster])
