#!/usr/bin/env python3
"""Build native modules for C/C++ and QVM parity before static integration."""
import argparse
import json
from pathlib import Path
import re
import shlex
import subprocess

from run import ROOT, ENV


def build_modules(output, cc='cc', modules=('game', 'cgame', 'ui'), cxx='c++', language='c', content='quake3'):
    if content == 'openarena':
        from openarena_native import build_modules as build_openarena
        return build_openarena(output, cc, modules, cxx, language)
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    compiler = shlex.split(cc if language == 'c' else cxx)
    mode = ['-x', 'c', '-std=gnu99'] if language == 'c' else ['-x', 'c++', '-std=c++20', '-fno-exceptions', '-fno-rtti', '-Werror=write-strings', '-Werror=register', '-U_GNU_SOURCE', '-D_DEFAULT_SOURCE']
    version = subprocess.check_output([*compiler, '--version'], text=True)
    frozen = json.loads((ROOT / 'tests/native-warnings.json').read_text())
    warnings = ['-Wall', '-Wextra', '-Werror', *['-Wno-' + name[2:] for name in frozen['clang' if 'clang' in version.lower() else 'gcc']]]
    manifest = json.loads((ROOT / 'docs/native-game-import.json').read_text())
    layouts = []
    for name, command in [
        ('native', [*compiler, *mode, '-include', 'code/game/native_abi.h']),
        ('engine', [*shlex.split(cxx), '-x', 'c++', '-std=c++20', '-DENGINE'])
    ]:
        binary = output / ('layout-' + name)
        subprocess.run([*command, 'tests/probes/native_layout.c', '-o', str(binary)],
                       cwd=ROOT, env=ENV, check=True)
        layout = subprocess.check_output([binary], env=ENV)
        (output / ('layout-' + name + '.txt')).write_bytes(layout)
        layouts.append(layout)
    if layouts[0] != layouts[1]:
        raise RuntimeError('native/engine ABI layout differs; see layout-*.txt')
    flags = [*compiler, *mode, *warnings, '-fPIC', '-O2',
             '-ffp-contract=off', '-fno-strict-aliasing', '-fwrapv', '-fno-builtin',
             '-include', 'code/game/native_abi.h']
    library = ['code/game/bg_lib.cpp']
    if language == 'c++' and 'clang' in version.lower():
        # glibc's inline atof conflicts with this file's compatibility definition.
        obj = output / 'bg_lib.o'
        command = [*flags, '-D__NO_INLINE__', '-c', 'code/game/bg_lib.cpp', '-o', str(obj)]
        (output / 'bg_lib.command').write_text(shlex.join(command) + '\n')
        with (output / 'bg_lib.log').open('w') as log:
            subprocess.run(command, cwd=ROOT, env=ENV, stdout=log, stderr=subprocess.STDOUT, check=True)
        library = ['-x', 'none', str(obj)]
    binaries = {}
    for module in modules:
        binary = output / (('qagame' if module == 'game' else module) + 'x86_64.so')
        command = [*flags, '-shared',
                   '-D' + {'game': 'QAGAME', 'cgame': 'CGAME', 'ui': 'UI'}[module],
                   '-Wl,-Bsymbolic,-z,defs',
                   *manifest['modules'][module], *library, '-lm', '-o', str(binary)]
        (output / (module + '.command')).write_text(shlex.join(command) + '\n')
        with (output / (module + '.log')).open('w') as log:
            subprocess.run(command, cwd=ROOT, env=ENV, stdout=log, stderr=subprocess.STDOUT, check=True)
        binaries[module] = binary
    return binaries


def normalize_log(log):
    # Implementation/build metadata and the old VM printf's extra numeric padding.
    log = re.sub(rb'^\.\.\.which has vmMagic VM_MAGIC_VER2\r?\n|^Loading [0-9]+ jump table targets\r?\n|^\^3jump target [0-9]+ set on instruction [0-9]+ \(OP_CVIF\) with bad opStack [0-9]+\r?\n', b'', log, flags=re.M)
    log = re.sub(rb"^(?:Loading vm file vm/qagame\.qvm\.\.\.|VM file qagame compiled to [0-9]+ bytes of code|qagame loaded in [0-9]+ bytes on the hunk|Loading dll file qagame\.|VM_LoadDLL 'qagamex86_64\.so' ok|VM_LoadDll\(qagame\) found \*\*vmMain\*\* at 0x[0-9a-fA-F]+|VM_LoadDll\(qagame\) succeeded!|gamedate:.*)\r?\n", b'', log, flags=re.M)
    return re.sub(rb'(\\skill\\) +(?=[0-9]+\.[0-9]+\\)', rb'\1', log)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='cc')
    parser.add_argument('--cxx', default='c++')
    parser.add_argument('--language', choices=['c', 'c++'], default='c')
    parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-native'))
    args = parser.parse_args()
    for module, binary in build_modules(args.output, args.cc, cxx=args.cxx, language=args.language, content=args.content).items():
        print(module, binary)
