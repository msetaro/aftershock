#!/usr/bin/env python3
"""Permanent regression checks; golden writes require --regenerate outside CI."""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SOURCES = 'q_shared q_math msg huffman huffman_static cmd cvar files net_chan net_ip md4 cm_load cm_patch cm_polylib cm_test cm_trace'.split()
ENV = dict(os.environ, SOURCE_DATE_EPOCH='1789257600', LC_ALL='C')


def run(args, **kwargs):
    return subprocess.run([str(a) for a in args], cwd=ROOT, env=ENV, check=True, **kwargs)


def compare(name, actual, regenerate):
    path = ROOT / 'tests/golden' / name
    if regenerate:
        if os.environ.get('CI'):
            raise SystemExit('FAIL: CI must never regenerate goldens')
        path.write_bytes(actual)
        print('GENERATED', path.relative_to(ROOT), hashlib.sha256(actual).hexdigest())
        return
    if not path.is_file():
        raise SystemExit('FAIL: missing reviewed golden: ' + name)
    expected = path.read_bytes()
    if expected != actual:
        print(''.join(difflib.unified_diff(expected.decode().splitlines(True), actual.decode().splitlines(True), fromfile=str(path), tofile='actual')))
        raise SystemExit('FAIL: ' + name)
    print('PASS', name, hashlib.sha256(actual).hexdigest())


def build(output, variables, targets=()):
    output.mkdir(parents=True, exist_ok=True)
    settings = json.dumps(variables)
    manifest = output / 'settings.json'
    refresh = [] if manifest.exists() and manifest.read_text() == settings else ['-B']
    command = ['make', '-j8', *refresh, f'BUILD_DIR={output}', *variables, *targets]
    with (output / 'build.log').open('w') as log:
        result = subprocess.run([str(a) for a in command], cwd=ROOT, env=ENV, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        print((output / 'build.log').read_text())
        result.check_returncode()
    manifest.write_text(settings)
    return output / 'release-linux-x86_64'


def differential(args):
    objects = args.output / 'unit-build/release-linux-x86_64/ded'
    instrument = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if args.sanitize else []
    variables = [f'CC={args.cc}', f'CXX={args.cxx}', 'BUILD_CLIENT=0', 'USE_SDL=0', 'USE_CURL=0', 'CFLAGS=-ffunction-sections -fdata-sections ' + ' '.join(instrument)]
    build(args.output / 'unit-build', variables, [objects / (s + '.o') for s in SOURCES])
    binary = args.output / 'differential'
    run([*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti', '-O2',
         *instrument, '-ffunction-sections', '-fdata-sections', 'tests/probes/differential.cpp',
         *[objects / (s + '.o') for s in SOURCES], '-Wl,--gc-sections',
         '-Wl,--wrap=_Z11FS_ReadFilePKcPPv', '-o', binary, '-lm'])
    command = [binary]
    if args.check == 'differential':
        name = content_maps(args.content)[0] + '.bsp'
        bsp = None
        for archive in sorted(args.data.glob('*.pk3')):
            with zipfile.ZipFile(archive) as pak:
                if 'maps/' + name in pak.namelist():
                    bsp = pak.read('maps/' + name)
        if bsp is None:
            raise SystemExit('FAIL: missing map: ' + name)
        map_path = args.output / name
        map_path.write_bytes(bsp)
        command.append(map_path)
        print('BSP SHA256', hashlib.sha256(bsp).hexdigest())
    result = subprocess.run([str(a) for a in command], cwd=ROOT, env=ENV, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    diagnostics = result.stderr.decode()
    if diagnostics:
        print(diagnostics, end='')
    result.check_returncode()
    if args.known_bugs:
        check_known_bugs(diagnostics)
    actual = result.stdout
    (args.output / (args.check + '.txt')).write_bytes(actual)
    compare(('openarena/' if args.check == 'differential' and args.content == 'openarena' else '') + args.check + '.txt', actual, args.regenerate)
    if args.negative_control:
        negative_control(args, objects, binary)


def check_known_bugs(diagnostics):
    patterns = [line for line in (ROOT / 'tests/known-bugs.txt').read_text().splitlines()
                if line and not line.startswith('#')]
    errors = [line for line in diagnostics.splitlines() if 'runtime error:' in line]
    for error in errors:
        if not any(re.search(pattern, error) for pattern in patterns):
            raise SystemExit('FAIL: unlisted sanitizer failure: ' + error)
        print('known, tracked in #31: ' + error)
    for pattern in patterns:
        if not any(re.search(pattern, error) for error in errors):
            raise SystemExit('FAIL: listed bug stopped failing; remove its entry: ' + pattern)


def negative_control(args, objects, binary):
    original = (ROOT / 'code/qcommon/q_math.cpp').read_text()
    begin = original.index('float Q_rsqrt( float number )\n{')
    end = original.index('float Q_fabs', begin)
    body = original[begin:end]
    assert body.count('return 1.0f / sqrtf( number );') == 1
    mutant = args.output / 'q_math-one-ulp.cpp'
    mutant.write_text(original[:begin] + body.replace('return 1.0f / sqrtf( number );', 'return nextafterf( 1.0f / sqrtf( number ), INFINITY );') + original[end:])
    target = objects / 'q_math.o'
    recipe = run(['make', '-Bn', 'V=1', f'BUILD_DIR={args.output / "unit-build"}',
                  f'CC={args.cc}', f'CXX={args.cxx}', str(target)], stdout=subprocess.PIPE).stdout.decode()
    commands = [shlex.split(line) for line in recipe.splitlines() if ' -c code/qcommon/q_math.cpp' in line]
    assert len(commands) == 1
    command = commands[0]
    command[command.index('code/qcommon/q_math.cpp')] = str(mutant)
    obj = args.output / 'q_math-one-ulp.o'
    command[command.index('-o') + 1] = str(obj)
    run([*command, '-Icode/qcommon', '-ffunction-sections', '-fdata-sections'])
    mutated_binary = args.output / 'differential-one-ulp'
    run([*shlex.split(args.cxx), '-std=c++20', '-fno-exceptions', '-fno-rtti', '-O2',
         '-ffunction-sections', '-fdata-sections', 'tests/probes/differential.cpp',
         *[obj if stem == 'q_math' else objects / (stem + '.o') for stem in SOURCES],
         '-Wl,--gc-sections', '-Wl,--wrap=_Z11FS_ReadFilePKcPPv', '-o', mutated_binary, '-lm'])
    changed = run([mutated_binary], stdout=subprocess.PIPE).stdout
    expected = (ROOT / 'tests/golden/unit.txt').read_bytes()
    if changed == expected:
        raise SystemExit('FAIL: one-ULP Q_rsqrt mutation escaped the golden check')
    (args.output / 'one-ulp.diff').write_text(''.join(difflib.unified_diff(expected.decode().splitlines(True), changed.decode().splitlines(True))))
    print('PASS: one-ULP Q_rsqrt mutation rejected; engine source untouched')


def content_maps(content):
    return ('oa_dm1', 'oa_dm7') if content == 'openarena' else ('q3dm17', 'q3dm7')


def content_bots(content):
    return ('sarge', 'major') if content == 'quake3' else ('sarge', 'beret')


def content_settings(content):
    return ['+set', 'fs_game', 'baseoa', '+set', 'net_enabled', '0'] if content == 'openarena' else []


def runtime(args):
    binary = build(args.output / 'runtime-build', [f'CC={args.cc}', f'CXX={args.cxx}', 'BUILD_CLIENT=0']) / 'quake3e.ded.x64'
    for map_name in content_maps(args.content):
        results = []
        # Isolated home prevents the user's config and pak cache influencing fixtures.
        with tempfile.TemporaryDirectory(prefix='aftershock-smoke-') as home:
            base = Path(home) / ('baseoa' if args.content == 'openarena' else 'baseq3')
            base.mkdir()
            for pak in args.data.glob('*.pk3'):
                (base / pak.name).symlink_to(pak)
            if not list(base.glob('*.pk3')):
                raise SystemExit('FAIL: installed content paks are required')
            command = ['timeout', '90', 'faketime', '-f', '@2026-01-01 00:00:00 i0.01', binary,
                       '+set', 'fs_basepath', home, '+set', 'fs_homepath', home,
                       *content_settings(args.content),
                       '+set', 'dedicated', '1', '+set', 'sv_pure', '0', '+set', 'com_logfile', '0',
                       '+map', map_name, '+addbot', content_bots(args.content)[0], '3', '+addbot', content_bots(args.content)[1], '3', '+wait', '900' if args.content == 'openarena' else '300', '+quit']
            for iteration in ('warmup', '1', '2'):
                result = subprocess.run([str(a) for a in command], cwd=ROOT, env=ENV, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                log = result.stdout
                (args.output / f'{map_name}-{iteration}.log').write_bytes(log)
                result.check_returncode()
                # Only installation metadata is normalized; gameplay text is retained.
                normalized = re.sub(rb'^\.\.\.found [0-9]+ cached paks\r?\n|^Working directory:.*\r?\n', b'', log, flags=re.M)
                normalized = normalized.replace(home.encode(), b'<HOME>').replace(str(args.data.parent).encode(), b'<DATA>')
                if args.content == 'openarena':
                    normalized = re.sub(rb'^\.\.\.detecting CPU, found .*$', b'...detecting CPU, found <CPU>', normalized, flags=re.M)
                if iteration != 'warmup':
                    results.append(normalized)
            if results[0] != results[1]:
                raise SystemExit('FAIL: repeated runtime differs: ' + map_name)
            if b'ClientBegin: 1' not in results[0] or b'Kill:' not in results[0]:
                raise SystemExit('FAIL: runtime did not exercise both bots: ' + map_name)
            compare(('openarena/' if args.content == 'openarena' else '') + map_name + '.log', results[0], args.regenerate)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('check', choices=['unit', 'differential', 'runtime'])
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-tests'))
    parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
    parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
    parser.add_argument('--known-bugs', action='store_true')
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--cxx', default='g++')
    parser.add_argument('--sanitize', action='store_true')
    parser.add_argument('--negative-control', action='store_true')
    parser.add_argument('--regenerate', action='store_true', help='explicitly replace goldens; prohibited in CI')
    args = parser.parse_args()
    if args.regenerate and os.environ.get('CI'):
        parser.error('CI must never regenerate goldens')
    if args.negative_control and (args.check != 'unit' or args.sanitize or args.regenerate):
        parser.error('--negative-control requires unit without regeneration/sanitizers')
    if args.sanitize and args.check == 'runtime':
        parser.error('runtime sanitizer runner is not implemented yet')
    if args.known_bugs and (not args.sanitize or args.regenerate):
        parser.error('--known-bugs requires sanitizers without regeneration')
    if args.sanitize:
        ENV['ASAN_OPTIONS'] = 'detect_leaks=0:halt_on_error=1'
        ENV['UBSAN_OPTIONS'] = f'halt_on_error={0 if args.known_bugs else 1}:suppressions={ROOT / "tools/port/ubsan.supp"}'
    args.output = args.output.resolve()
    args.data = args.data.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    if args.check == 'runtime':
        runtime(args)
    else:
        differential(args)


if __name__ == '__main__':
    main()
