#!/usr/bin/env python3
"""Compare fresh pinned shader compilation with the committed offline cache."""
import argparse
import json
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
    print(f'PASS: all {len(package["shaders"])} shader binaries and interfaces match; package {package["sha256"]}')


if __name__ == '__main__':
    main()
