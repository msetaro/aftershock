#!/usr/bin/env python3
"""#5 migration oracle: compare raw Make/CMake objects at the recorded checkpoint."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import os

ROOT = Path(__file__).resolve().parents[2]
ENV = dict(os.environ, SOURCE_DATE_EPOCH='1789257600', LC_ALL='C')


def objects(folder, cmake):
    result = {}
    for row in json.loads((folder / 'compile_commands.json').read_text()):
        args = row.get('arguments') or shlex.split(row['command'])
        output = Path(row.get('output') or args[args.index('-o') + 1])
        if not output.is_absolute():
            output = Path(row['directory']) / output
        if not cmake:
            key = '/'.join(output.relative_to(folder).parts[1:])
        else:
            target = output.parts[output.parts.index('CMakeFiles') + 1][:-4]
            source = Path(row['file'])
            if target.startswith('native_'):
                module = next(a for a in args if a.startswith('-DNATIVE_NAMESPACE=')).split('=', 1)[1]
                native = next(a for a in args if a.startswith('-DNATIVE_SOURCE=')).split('=', 1)[1].strip('"')
                key = 'native/' + module + '-' + Path(native).stem + '.o'
            else:
                part = {'server': 'ded', 'client': 'client', 'resources': 'client',
                        'opengl': 'rend1', 'vulkan': 'rendv'}[target]
                for vendor, name in [('libjpeg', 'jpeg'), ('libogg', 'ogg'), ('libvorbis', 'vorbis')]:
                    if vendor in source.parts:
                        part += '/' + name
                key = part + '/' + source.stem + '.o'
        if key in result:
            raise RuntimeError('duplicate object: ' + key)
        result[key] = hashlib.sha256(output.read_bytes()).hexdigest()
    if not cmake:
        for output in folder.glob('*/client/win_resource.o'):
            result['client/win_resource.o'] = hashlib.sha256(output.read_bytes()).hexdigest()
    if not result:
        raise RuntimeError('no objects found: ' + str(folder))
    (folder / 'object-hashes.json').write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--config', choices=['Release', 'Debug'], default='Release')
    parser.add_argument('--make-setting', action='append', default=[])
    parser.add_argument('--cmake-setting', action='append', default=[])
    args = parser.parse_args()
    args.output = args.output.resolve()
    before, after = args.output / 'make', args.output / 'cmake'
    before.mkdir(parents=True, exist_ok=True)
    settings = ['BUILD_DIR=' + str(before), *args.make_setting]
    commands = [
        ['make', '-j8', *settings, args.config.lower()],
        ['cmake', '-S', str(ROOT), '-B', str(after), '-G', 'Ninja',
         '-DCMAKE_BUILD_TYPE=' + args.config, *['-D' + value for value in args.cmake_setting]],
        ['cmake', '--build', str(after), '-j8'],
    ]
    with (args.output / 'build.log').open('w') as log:
        for command in commands:
            log.write(shlex.join(command) + '\n')
            log.flush()
            subprocess.run(command, cwd=ROOT, env=ENV, stdout=log, stderr=subprocess.STDOUT, check=True)
    recipe = subprocess.check_output(['make', '-Bn', 'V=1', *settings, args.config.lower()], cwd=ROOT, env=ENV, text=True)
    rows = []
    for line in recipe.splitlines():
        if ' -c ' not in line:
            continue
        command = shlex.split(line)
        rows.append({'directory': str(ROOT), 'arguments': command,
                     'file': command[command.index('-c') + 1], 'output': command[command.index('-o') + 1]})
    (before / 'compile_commands.json').write_text(json.dumps(rows, indent=2) + '\n')
    reference, candidate = objects(before, False), objects(after, True)
    differences = {key: {'make': reference.get(key), 'cmake': candidate.get(key)}
                   for key in sorted(reference.keys() | candidate.keys()) if reference.get(key) != candidate.get(key)}
    (args.output / 'differences.json').write_text(json.dumps(differences, indent=2) + '\n')
    if differences:
        raise SystemExit(f'FAIL: {len(differences)} object differences; see {args.output}/differences.json')
    print(f'PASS: {len(reference)}/{len(candidate)} raw object hashes identical; {args.output}')


if __name__ == '__main__':
    main()
