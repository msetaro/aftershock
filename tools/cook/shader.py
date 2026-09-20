"""Compile project GLSL with the same pinned compiler as the engine package."""
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

import model

ROOT = Path(__file__).resolve().parents[2]


def cook(source, root, options, read):
    manifest = json.loads((ROOT / 'engine/renderervk/shaders/manifest.json').read_text())
    compiler = os.environ.get('AFTERSHOCK_GLSLANG') or shutil.which('glslang') or shutil.which('glslangValidator')
    if not compiler:
        raise ValueError('shader cooking requires pinned glslang ' + manifest['compiler_version'] + '; set AFTERSHOCK_GLSLANG')
    version = subprocess.check_output([compiler, '--version'], text=True)
    if not re.search(r'^Glslang Version: \d+:' + re.escape(manifest['compiler_version']) + r'$', version, re.M):
        raise ValueError('shader cooking requires glslang ' + manifest['compiler_version'])
    stage = options.get('stage', source.suffix.lstrip('.'))
    defines = options.get('defines', [])
    if stage not in ('vert', 'frag') or not isinstance(defines, list) or any(not isinstance(flag, str) or not re.fullmatch(r'[A-Za-z_]\w*(=[A-Za-z0-9_.+-]+)?', flag) for flag in defines):
        raise ValueError('shader requires vert/frag stage and simple NAME or NAME=value defines')
    inputs, pending = {}, [source]
    while pending:
        path = pending.pop().resolve()
        if path in inputs:
            continue
        text = read(path).decode('utf-8').replace('\r\n', '\n')
        inputs[path] = text
        for directive in re.findall(r'^\s*#\s*include\s+([^\n]+)', text, re.M):
            include = re.fullmatch(r'"([^"\n]+)"\s*(?://.*)?', directive)
            if not include:
                raise ValueError('shader includes must use literal quoted project paths')
            pending.append(path.parent / include[1])
    with tempfile.TemporaryDirectory(prefix='aftershock-shader-') as temporary:
        directory = Path(temporary)
        for path, text in inputs.items():
            target = directory / path.relative_to(root)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text)
        binary = directory / 'cooked.spv'
        result = subprocess.run([compiler, '-S', stage, *manifest['options'], '-o', str(binary),
                                 str(directory / source.relative_to(root)), *['-D' + flag for flag in defines]],
                                cwd=directory, capture_output=True, text=True)
        if result.returncode:
            raise ValueError('shader compilation failed:\n' + result.stdout + result.stderr)
        payload = binary.read_bytes()
    spec = importlib.util.spec_from_file_location('shader_build', ROOT / 'tools/shaders/build.py')
    build = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(build)
    if build.layout(payload)['entry_points'] != [{'stage': stage, 'entry': 'main'}]:
        raise ValueError('shader must expose one main entry point for its declared stage')
    return model.wrapped(b'ASSPV\0\0\0', payload)
