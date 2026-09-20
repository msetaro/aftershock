#!/usr/bin/env python3
"""Compare fresh pinned shader compilation with the committed offline cache."""
import argparse
import json
import hashlib
import os
import struct
import importlib.util
import tempfile
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', required=True, help='glslang 16.6.0 executable')
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-shader-tests'))
    args = parser.parse_args()
    for mode in ('cached', 'compiled'):
        command = [sys.executable, 'tools/shaders/build.py', '--output', str(args.output / mode)]
        if mode == 'compiled':
            command += ['--compile', '--compiler', args.compiler]
        subprocess.run(command, cwd=ROOT, check=True)
    for filename in ('shader_package.h', 'shader_package.json'):
        if (args.output / 'cached' / filename).read_bytes() != (args.output / 'compiled' / filename).read_bytes():
            raise SystemExit('FAIL: fresh shader compilation differs from committed cache: ' + filename)
    package = json.loads((args.output / 'compiled/shader_package.json').read_text())
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location('shader_build', ROOT / 'tools/shaders/build.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    with tempfile.TemporaryDirectory(prefix='aftershock-shader-inputs-') as temporary:
        module.SOURCE = Path(temporary)
        source = module.SOURCE / 'main.vert'
        include = module.SOURCE / 'common.glsl'
        source.write_text('#include "common.glsl"\nvoid main() {}\n')
        include.write_text('const int value = 1;\n')
        before = module.source_inputs(source)
        include.write_text('const int value = 2;\n')
        after = module.source_inputs(source)
        assert before['main.vert'] == after['main.vert'] and before['common.glsl'] != after['common.glsl']
        row = dict(package['shaders'][0], includes=before)
        key = module.recipe_hash(package, row)
        assert key != module.recipe_hash(package, dict(row, includes=after))
        assert key != module.recipe_hash(dict(package, options=['-V', '-g']), row)
        assert key != module.recipe_hash(dict(package, compiler_version='changed'), row)
    with tempfile.TemporaryDirectory(prefix='aftershock-cooked-shader-') as temporary:
        directory = Path(temporary)
        source = (ROOT / 'engine/renderervk/shaders/color.vert').read_text()
        source = source.replace('#version 450', '#version 450\n#extension GL_GOOGLE_include_directive : require\n#include "factor.glsl"')
        (directory / 'owned.vert').write_text(source.replace('in_position, 1.0', 'in_position, POSITION_W'))
        include = directory / 'factor.glsl'
        include.write_text('#define POSITION_W 1.0\n')
        project = directory / 'assets.json'
        project.write_text(json.dumps({'version': 1, 'assets': [
            {'name': 'shaders/color_vert_spv', 'kind': 'shader', 'source': 'owned.vert', 'stage': 'vert'}]}))
        cooked = directory / 'cooked'
        command = [sys.executable, 'tools/cook', str(project), '--output', str(cooked)]
        env = dict(os.environ, AFTERSHOCK_GLSLANG=str(Path(args.compiler).resolve()))
        first = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True)
        assert first.returncode == 0, first.stdout + first.stderr
        asset = cooked / 'shaders/color_vert_spv.asspv'
        before = asset.read_bytes()
        magic, version, size, digest = struct.unpack_from('<8sII32s', before)
        assert magic == b'ASSPV\0\0\0' and version == 1 and size == len(before) - 48
        assert digest == hashlib.sha256(before[48:]).digest()
        manifest = json.loads((cooked / 'shaders/color_vert_spv.manifest.json').read_text())
        assert {row['path'] for row in manifest['inputs']} == {'owned.vert', 'factor.glsl'}
        unchanged = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True, check=True)
        assert json.loads(unchanged.stdout)['skipped'] == ['shaders/color_vert_spv']
        include.write_text('#define POSITION_W 2.0\n')
        subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True, check=True)
        after = asset.read_bytes()
        assert after != before
        custom = directory / 'package'
        subprocess.run([sys.executable, 'tools/shaders/build.py', '--output', str(custom), '--cooked', str(cooked)], cwd=ROOT, check=True)
        assert (custom / 'color_vert_spv.spv').read_bytes() == after[48:]
        for row in package['shaders']:
            if row['name'] != 'color_vert_spv':
                assert (custom / (row['name'] + '.spv')).read_bytes() == (args.output / 'cached' / (row['name'] + '.spv')).read_bytes()
        custom_package = json.loads((custom / 'shader_package.json').read_text())
        assert custom_package['sha256'] != package['sha256']
    print(f'PASS: all {len(package["shaders"])} shader binaries and interfaces match; package {package["sha256"]}')


if __name__ == '__main__':
    main()
