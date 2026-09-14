#!/usr/bin/env python3
"""Reject objects needing destruction across the engine's longjmp error path."""
import argparse
import json
from pathlib import Path
import re
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
CORE = ('qcommon', 'client', 'server', 'botlib', 'renderercommon', 'renderer',
        'renderervk', 'cgame', 'game', 'ui')
LOCATION = 'isExpansionInFileMatching("(^|/)code/(' + '|'.join(CORE) + ')/")'
# clang-query's AST dump marks VarDecl/ParmVarDecl with needsDestruction as
# "destroyed". CXXBindTemporaryExpr represents a non-trivial temporary destructor.
BAD = re.compile(r'^(?:(?:Parm)?VarDecl\b.*\bdestroyed\b|CXXBindTemporaryExpr\b)', re.M)


def query(tool, files, flags, location, log):
    command = [tool, '-c', 'set output dump',
               '-c', f'match varDecl({location})',
               '-c', f'match cxxBindTemporaryExpr({location})', *files, *flags]
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True)
    log.write_text(result.stdout + result.stderr)
    if result.returncode or not re.search(r'\d+ matches?\.', result.stdout):
        raise RuntimeError(f'Clang analysis failed; see {log}')
    return BAD.findall(result.stdout)


def self_check(tool, output):
    source = output / 'control.cpp'
    source.write_text('''#include <string>
struct Plain { int value; ~Plain() = default; };
struct Owned { ~Owned() {} };
struct Derived : Owned {};
using Alias = Derived;
Owned global;
void check(Owned parameter) {
    Plain plain; Plain *pointer = &plain;
    Alias inherited; Owned array[2]; static Owned persistent;
    std::string text; Owned();
}
''')
    bad = query(tool, [str(source)], ['--', '-std=c++20', '-fno-exceptions', '-fno-rtti'],
                'isExpansionInMainFile()', output / 'negative.log')
    assert len(bad) == 7, f'negative control: expected 7 rejected objects, got {len(bad)}'
    source.write_text('''struct Plain { int value; ~Plain() = default; };
Plain global;
void check(Plain parameter) { Plain local[2]; Plain *pointer = local; Plain(); }
''')
    assert not query(tool, [str(source)], ['--', '-std=c++20'],
                     'isExpansionInMainFile()', output / 'positive.log')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clang-query', default=shutil.which('clang-query') or
                        shutil.which('clang-query-21'))
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-lifetimes'))
    args = parser.parse_args()
    if not args.clang_query:
        parser.error('clang-query is required (CI installs clang-tools)')
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    self_check(args.clang_query, args.output)
    commands = []
    # Read the supported build's actual flags; no parallel build or engine edits.
    for renderer in ('opengl', 'vulkan'):
        recipe = subprocess.check_output(
            ['make', '-Bn', 'V=1', 'CC=clang', 'CXX=clang++', 'USE_RENDERER_DLOPEN=0',
             f'RENDERER_DEFAULT={renderer}', f'BUILD_DIR={args.output / renderer}'],
            cwd=ROOT, text=True)
        for line in recipe.splitlines():
            if ' -c code/' not in line:
                continue
            command = shlex.split(line)
            source = Path(command[command.index('-c') + 1])
            if source.suffix != '.cpp' or source.parts[1] not in CORE:
                continue
            commands.append({'directory': str(ROOT), 'file': str(source),
                             'arguments': [a for a in command if a != '-MMD']})
    if not commands:
        raise RuntimeError('no engine compilation commands found')
    (args.output / 'compile_commands.json').write_text(json.dumps(commands))
    files = sorted({c['file'] for c in commands})
    bad = query(args.clang_query, files, ['-p', str(args.output)], LOCATION,
                args.output / 'engine.log')
    if bad:
        raise RuntimeError('non-trivial engine lifetimes:\n' + '\n'.join(bad))
    print(f'PASS: {len(files)} engine translation units, both renderer configurations; '
          'positive and seven-object negative controls passed')


if __name__ == '__main__':
    main()
