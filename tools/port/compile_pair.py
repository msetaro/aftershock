#!/usr/bin/env python3
"""Build G2/G3 objects and G4 -O2 assembly with actual Makefile TU flags."""
import os
from pathlib import Path
import shlex
import subprocess
import sys

if len(sys.argv) < 3:
    sys.exit('usage: compile_pair.py ded/md4.o OUTPUT_DIR [MAKE_VARIABLE=value ...]')
obj, output, *variables = sys.argv[1:]
settings = dict(v.split('=', 1) for v in variables if '=' in v)
platform = settings.get('PLATFORM', 'linux')
arch = settings.get('ARCH', 'x86_64')
output = Path(output).resolve()
output.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, SOURCE_DATE_EPOCH='1789257600', LC_ALL='C')
for mode, tag in [('0', 'c'), ('1', 'cxx')]:
    build = output / ('make-' + tag)
    target = build / f'release-{platform}-{arch}' / obj
    recipe = subprocess.check_output(
        ['make', '-Bn', 'V=1', f'BUILD_CXX={mode}', f'BUILD_DIR={build}', *variables, str(target)],
        text=True, env=env)
    commands = [shlex.split(line) for line in recipe.splitlines() if ' -c code/' in line]
    commands = [args for args in commands if '-o' in args and args[args.index('-o') + 1] == str(target)]
    if len(commands) != 1:
        sys.exit(f'FAIL: expected one compiler command for {obj}, got {len(commands)}')
    args = commands[0]
    index = args.index('-o')
    del args[index:index + 2]
    args.remove('-c')
    args = [arg for arg in args if arg not in ('-MMD', '-MP')]
    # G3 isolates symbol declarations/references from libc macro and optimizer
    # substitutions and automatic helper inlining. Production G2 objects and G4 assembly keep their own flags.
    for suffix, flags in [('o', ['-g', '-fno-eliminate-unused-debug-types', '-c']),
                          ('sym.o', ['-g0', '-O2', '-fno-builtin', '-fno-inline-functions', '-D__NO_CTYPE=1', '-U_FORTIFY_SOURCE', '-c']),
                          ('s', ['-g0', '-O2', '-S'])]:
        artifact = output / f'{Path(obj).stem}.{tag}.{suffix}'
        # Inspect real machine code/DWARF, not serialized LTO IR.
        lto = ['-fno-lto'] if any(a.startswith('-flto') for a in args) else []
        command = [*args, *flags, *lto, '-o', str(artifact)]
        artifact.with_suffix(artifact.suffix + '.command').write_text(shlex.join(command) + '\n')
        subprocess.run(command, env=env, check=True)
        print(artifact)
