#!/usr/bin/env python3
"""Check production IQM joint scale/rotation and inverse composition."""
import argparse
import json
from pathlib import Path
import shlex
import tempfile

from cook import cook, source_assets
from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-iqm-scale'))
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
binary = args.output.resolve() / 'check'
run([*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti', '-O2',
     '-DUSE_VULKAN_API', '-ffunction-sections', '-fdata-sections',
     '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/iqm_scale.cpp', 'engine/qcommon/q_shared.cpp',
     'engine/qcommon/q_math.cpp', '-Wl,--gc-sections', '-lm', '-o', binary])
run([binary])
with tempfile.TemporaryDirectory(prefix='aftershock-iqm-scale-') as temporary:
    source = Path(temporary)
    project, _ = source_assets(source)
    project.write_text(json.dumps({'version': 1, 'assets': [
        {'name': 'models/scaled', 'kind': 'model', 'source': 'nonuniform.gltf', 'scale': 1, 'fps': 2}]}))
    output = args.output.resolve() / 'cooked'
    cook(project, output)
    run([binary, output / 'models/scaled.iqm'])
