#!/usr/bin/env python3
"""Run remaining G1/G7 native configurations, retaining every command and log."""
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

os.chdir(Path(__file__).resolve().parents[2])
output = Path(sys.argv[1] if len(sys.argv) > 1 else '/tmp/aftershock-cpp-port/matrix').resolve()
output.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, SOURCE_DATE_EPOCH='1789257600', LC_ALL='C')
build = Path(tempfile.mkdtemp(prefix='build-', dir=output))
results = []
for compiler in ('gcc', 'clang'):
    for mode in (0, 1):
        # Default dlopen builds include both OpenGL and Vulkan and client/ded.
        configs = [('release-sdl', []), ('debug-nosdl', ['debug', 'USE_SDL=0'])]
        if compiler == 'gcc':
            configs += [('debug-sdl', ['debug']), ('release-nosdl', ['USE_SDL=0'])]
            configs += [('static-' + renderer, ['BUILD_SERVER=0', 'USE_RENDERER_DLOPEN=0',
                                                'RENDERER_DEFAULT=' + renderer])
                        for renderer in ('opengl', 'vulkan')]
        for config, variables in configs:
            name = f'{compiler}-c{mode}-{config}'
            command = ['make', '-k', '-j20', f'BUILD_CXX={mode}',
                       'CC=' + ('gcc' if compiler == 'gcc' else 'clang'),
                       'CXX=' + ('g++' if compiler == 'gcc' else 'clang++'),
                       'BUILD_DIR=' + str(build / name), *variables]
            log = output / (name + '.log')
            with log.open('w') as stream:
                stream.write(shlex.join(command) + '\n')
                stream.flush()
                status = subprocess.run(command, env=env, stdout=stream, stderr=subprocess.STDOUT).returncode
            errors = len(re.findall(r'\berror:', log.read_text()))
            results.append(dict(name=name, status=status, errors=errors, command=command))
            (output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
            print(f'{"PASS" if not status else "FAIL"}: {name}, exit {status}, {errors} compiler errors', flush=True)
sys.exit(any(result['status'] for result in results))
