#!/usr/bin/env python3
"""Build the pinned OpenArena C modules for hosted native/QVM parity."""
import argparse
import io
from pathlib import Path
import re
import shlex
import subprocess
import tarfile

from run import ROOT, ENV

REVISION = '331464ca396d80e91cf9be273588f2b5f4b7afc8'
REPOSITORY = 'https://github.com/OpenArena/gamecode'


def stage_source(output, source=Path('/tmp/aftershock-oa-native-source')):
    output = Path(output).resolve()
    source = Path(source).resolve()
    if not source.exists():
        subprocess.run(['git', 'init', source], check=True)
        subprocess.run(['git', '-C', source, 'fetch', '--depth=1', REPOSITORY, REVISION], check=True)
    archive = subprocess.check_output(['git', '-C', source, 'archive', REVISION, 'code', 'ui', 'Makefile'])
    output.mkdir(parents=True, exist_ok=True)
    with tarfile.open(fileobj=io.BytesIO(archive)) as package:
        package.extractall(output, filter='data')
    for name in ('openarena-name-comparison.patch', 'openarena-extension.patch',
                 'openarena-allocation-alignment.patch', 'openarena-free-list.patch'):
        subprocess.run(['git', 'apply', str(ROOT / 'tests/patches' / name)], cwd=output, check=True)
    return output


def build_modules(output, cc='cc', modules=('game', 'cgame', 'ui'), cxx='c++', language='c', static=False):
    if language != 'c':
        raise ValueError('the pinned OpenArena test dependency builds as C')
    output = stage_source(output)
    abi = (ROOT / 'game/bg/native_abi.h').read_text().replace('#define BASEGAME "baseq3"\n', '')
    (output / 'native_abi.h').write_text(abi)
    compiler = shlex.split(cc)
    version = subprocess.check_output([*compiler, '--version'], text=True)
    precision = '-cl-single-precision-constant' if 'clang' in version.lower() else '-fsingle-precision-constant'
    # OA appends eye vectors to refEntity; the engine consumes its 140-byte prefix.
    probe = (ROOT / 'tests/probes/native_layout.c').read_text()
    probe = probe.replace('../../game/bg/q_shared.h', '../../code/qcommon/q_shared.h')
    probe = probe.replace('../../game/cgame/tr_types.h', '../../code/renderer/tr_types.h')
    for name in ('botlib', 'be_aas', 'be_ai_goal', 'be_ai_move', 'be_ai_chat', 'be_ai_weap'):
        probe = probe.replace('../../game/game/' + name + '.h', '../../code/botlib/' + name + '.h')
    for module, prefix in (('game', 'g'), ('cgame', 'cg'), ('ui', 'ui')):
        probe = probe.replace('../../engine/public/' + prefix + '_public.h', '../../code/' + module + '/' + prefix + '_public.h')
    probe = probe.replace('#ifdef __cplusplus\n#define ALIGN', '#undef ALIGN\n#ifdef __cplusplus\n#define ALIGN')
    probe = probe.replace('SHOW(refEntity_t);', 'printf("refEntity_t %zu %zu\\n", offsetof(refEntity_t, eyepos), (size_t)ALIGN(refEntity_t));')
    probe = probe.replace('    printf("extension trap %d\\n", G_TRAP_GETVALUE);\n', '')
    path = output / 'tests/probes/native_layout.c'
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(probe)
    layouts = []
    for name, command in [
        ('native', [*compiler, '-x', 'c', '-std=gnu99', precision, '-include', str(output / 'native_abi.h'), str(path)]),
        ('engine', [*shlex.split(cxx), '-x', 'c++', '-std=c++20', '-DENGINE', 'tests/probes/native_layout.c'])
    ]:
        binary = output / ('layout-' + name)
        subprocess.run([*command, '-o', str(binary)], cwd=ROOT, env=ENV, check=True)
        layout = subprocess.check_output([binary], env=ENV)
        layout = re.sub(rb'^extension trap .*\n', b'', layout, flags=re.M)
        (output / ('layout-' + name + '.txt')).write_bytes(layout)
        layouts.append(layout)
    if layouts[0] != layouts[1]:
        raise RuntimeError('OpenArena/engine ABI layout differs; see layout-*.txt')
    make = (output / 'Makefile').read_text()
    binaries = {}
    for module in modules:
        prefix, variable = {'game': ('g', 'Q3GOBJ_'), 'cgame': ('cg', 'Q3CGOBJ_'), 'ui': ('ui', 'Q3UIOBJ_')}[module]
        block = make.split(variable + ' = \\\n')[1].split('\n\n')[0]
        sources = []
        for name in re.findall(r'\$\(B\)/\$\(BASEGAME\)/([^ ]+)\.o', block):
            path = Path('code') / (name + '.c')
            if path.name.startswith('bg_'):
                path = Path('code/game') / path.name
            elif module == 'ui' and path.parent.name == 'ui':
                path = Path('code/q3_ui') / path.name
            sources.append(str(path))
        main = output / ('code/q3_ui/ui_main.c' if module == 'ui' else 'code/' + module + '/' + prefix + '_main.c')
        text, count = re.subn(r'intptr_t vmMain\( int command,[^)]*\)', 'intptr_t vmMain( int command, int arg0, int arg1, int arg2 )', main.read_text())
        assert count == 1
        main.write_text(text)
        calls = Path('code') / module / (prefix + '_syscalls.c')
        text = (output / calls).read_text()
        edits = []
        for match in re.finditer(r'\bsyscall\s*\(', text):
            depth, end = 1, match.end()
            while depth:
                depth += (text[end] == '(') - (text[end] == ')')
                end += 1
            body = text[match.end():end-1]
            arguments, depth, start = [], 0, 0
            for index, char in enumerate(body):
                depth += (char == '(') - (char == ')')
                if char == ',' and depth == 0:
                    arguments.append(body[start:index].strip())
                    start = index + 1
            arguments.append(body[start:].strip())
            edits.append((match.start(), end, 'NATIVE_SYSCALL( ' + ', '.join('(intptr_t)( ' + a + ' )' for a in arguments) + ' )'))
        assert edits
        for start, end, replacement in reversed(edits):
            text = text[:start] + replacement + text[end:]
        (output / calls).write_text(text)
        sources.append(str(calls))
        binary = output / (module + '.o' if static else ('qagame' if module == 'game' else module) + 'x86_64.so')
        if static:
            public = ROOT / 'engine/public' / (prefix + '_native_public.h')
            direct = (ROOT / 'game' / module / (prefix + '_native.cpp')).read_text()
            if module == 'cgame':
                # This optional OA extension also fails in the original engine dispatch.
                direct += '\nvoid trap_R_LFX_ParticleEffect(int effect, const vec3_t origin, const vec3_t velocity) {\n'
                direct += '    CGameImport_Error("Unsupported native service: CG_R_LFX_PARTICLEEFFECT");\n}\n'
            (output / calls).write_text('#include "' + str(public) + '"\n' + direct)
            exports = (ROOT / 'game' / module / (prefix + '_native_exports.inc')).read_text()
            exports = exports.replace(module + '::', '')
            main.write_text(main.read_text() + '\n#include "' + str(public) + '"\n' + exports)
        command = [*compiler, '-x', 'c', '-std=gnu99', '-O2', '-fPIC', '-shared', precision,
                   '-ffp-contract=off', '-fno-strict-aliasing', '-fwrapv', '-fno-builtin',
                   '-include', 'native_abi.h', '-DPRODUCT_VERSION="1.35"',
                   '-D' + {'game': 'QAGAME', 'cgame': 'CGAME', 'ui': 'UI'}[module],
                   '-Wl,-Bsymbolic,-z,defs', *sources, str(ROOT / 'game/bg/bg_lib.cpp'),
                   '-I' + str(ROOT / 'game/bg'), '-lm', '-o', str(binary)]
        if static:
            command = [a for a in command if a not in ('-shared', '-Wl,-Bsymbolic,-z,defs', '-lm')]
            command += ['-r', '-nostdlib']
        (output / (module + '.command')).write_text(shlex.join(command) + '\n')
        with (output / (module + '.log')).open('w') as log:
            subprocess.run(command, cwd=output, env=ENV, stdout=log, stderr=subprocess.STDOUT, check=True)
        if static:
            # Preserve C compilation and each module's original private symbol scope.
            names = subprocess.check_output(['nm', '--defined-only', '--extern-only', binary], text=True)
            names = [line.split()[-1] for line in names.splitlines() if line.split()]
            symbols = output / (module + '.symbols')
            symbols.write_text(''.join(name + ' oa_' + module + '_' + name + '\n'
                                      for name in sorted(names)
                                      if not name.startswith(('Game_', 'NativeCGame_', 'NativeUI_'))))
            subprocess.run(['objcopy', '--redefine-syms=' + str(symbols), binary], check=True)
        binaries[module] = binary
    return binaries


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='gcc')
    parser.add_argument('--static', action='store_true', help='emit isolated C objects for the native engine')
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-openarena-native'))
    args = parser.parse_args()
    for module, binary in build_modules(args.output, args.cc, static=args.static).items():
        print(module, binary)
