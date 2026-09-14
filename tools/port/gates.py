#!/usr/bin/env python3
"""Compare C/C++ artifacts; preserve linkage, layout, and instruction differences."""
import difflib
import re
import shlex
import subprocess
import tempfile
import sys
from pathlib import Path


def run(*args):
    return subprocess.check_output(args, text=True)


def plain_symbol(name):
    name = re.sub(r'\(.*\)', '', name)
    name = re.sub(r' \[clone ([^]]+)\]', r'\1', name)
    # GCC numbers local static objects and optimization clones per TU.
    return re.sub(r'\.(\d+)(?=\.|$)', '', name)


def layout(path):
    if Path(path).read_bytes()[:2] == b'\x64\x86':
        # Resolve COFF DWARF relocations before converting for pahole. This
        # temporary carrier is never executed and is used only by G2; G3
        # always examines the original object and its real undefined symbols.
        with tempfile.TemporaryDirectory(prefix='port-layout-coff-') as directory:
            pe = str(Path(directory) / 'layout.exe')
            elf = str(Path(directory) / 'layout.elf')
            names = [line.split()[0] for line in run(
                'x86_64-w64-mingw32-nm', '-u', '--format=posix', path).splitlines()]
            run('x86_64-w64-mingw32-ld', '--image-base=0x10000000', '--entry=0',
                '--disable-auto-import', path, '-o', pe,
                *['--defsym=' + name + '=0' for name in names])
            run('objcopy', '-O', 'elf64-x86-64', pe, elf)
            return layout(elf)
    if '.debug_info' not in run('readelf', '-S', '-W', path):
        raise ValueError(f'{path}: missing DWARF; compile with -g -fno-eliminate-unused-debug-types')
    raw = run('pahole', '--sort', '-a', '-A', '-I', '-M', '--show_private_classes', path)
    records = []
    # Declaration provenance excludes only system/builtin/vendor records, never
    # missing engine records: each side is filtered independently before diff.
    for block in re.split(r'(?=/\* Used at:)', raw):
        declaration = re.search(r'/\* <[0-9a-f]+> (.+):\d+ \*/', block)
        if not declaration:
            continue
        source = declaration[1]
        if not re.search(r'(?:^|/)code/(?!libjpeg/|libogg/|libvorbis/|libcurl/|libsdl/|renderer2/)', source):
            continue
        block = re.sub(r'/\* Used at:.*?\*/\n|/\* <[0-9a-f]+> .*?\*/\n', '', block)
        records.append(re.sub(r'[ \t]+', ' ', block).strip() + '\n')
    if not records:
        raise ValueError(f'{path}: no engine layouts found; this is not an ABI pass')
    return ''.join(sorted(records)).splitlines(keepends=True)


def renderer_mode(path):
    command = Path(path + '.command')
    if not command.exists():
        return None
    return any(arg == '-DUSE_RENDERER_DLOPEN' or arg.startswith('-DUSE_RENDERER_DLOPEN=')
               for arg in shlex.split(command.read_text()))


def symbols(path, renderer_boundary=True):
    rows = []
    header = Path(path).read_bytes()[:20]
    arm_mapping = header[:4] == b'\x7fELF' and int.from_bytes(
        header[18:20], 'little' if header[5] == 1 else 'big') in (40, 183)
    for line in run('nm', '--format=posix', path).splitlines():
        fields = line.split()
        name, kind = fields[:2]
        if arm_mapping and kind.islower() and re.fullmatch(r'\$[adtx](?:\.\d+)?', name):
            continue  # ARM instruction/data mapping metadata, not source symbols.
        if name.startswith(('.L', '__func__.', '__FUNCTION__.', '__PRETTY_FUNCTION__.')) or kind in ('a', 'N'):
            continue  # compiler labels, function-name strings, debug/file metadata
        # COFF also embeds function names in .text$/.pdata$/.xdata$
        # section symbols; retain those records after the same demangling.
        demangled = re.sub(r'_Z\w+', lambda match: run('c++filt', match[0]).strip(), name)
        # Local statics have function scope in C++ demangling only.
        normalized = plain_symbol(demangled).split('::')[-1]
        boundary_name = normalized.split('.')[0]
        boundary = boundary_name in {
            'GetRefAPI', 'dllEntry', 'vmMain', 'snd_p', 'snd_out',
            'snd_linear_count', 'Q_setjmp_c', 'Q_longjmp_c', 'CPUID_EX',
            'Q_GetFPUCW', 'Q_SetFPUCW', '__clear_cache', 'NvOptimusEnablement',
            'AmdPowerXpressRequestHighPerformance',
        } or boundary_name.startswith('S_WriteLinearBlastStereo16_')
        if boundary_name == 'GetRefAPI' and not renderer_boundary:
            boundary = False  # Static renderer calls stay within the C++ build.
        if boundary_name == 'CPUID_EX' and kind == 't':
            boundary = False  # GNU private helper is not the MSVC assembly entry.
        if boundary:
            normalized = re.sub(r'\.(\d+)(?=\.|$)', '', name)  # Preserve raw ABI spelling, ignoring clone numbering.
        rows.append(f'{kind} {normalized}\n')
    if not rows:
        raise ValueError(f'{path}: no defined symbols')
    return sorted(rows)


def codegen(path):
    text = Path(path).read_text()
    for symbol in sorted(set(re.findall(r'\b_Z\w+', text)), key=len, reverse=True):
        text = text.replace(symbol, plain_symbol(run('c++filt', symbol).strip()))
    labels = {}
    def label(match):
        key = match[0]
        if key not in labels:
            labels[key] = f'.Lnorm{len(labels)}'
        return labels[key]
    text = re.sub(r'\.L[A-Za-z]*\d+\b', label, text)
    return [line.rstrip() + '\n' for line in text.splitlines()
            if not re.match(r'\s*\.(file|ident|loc)\b', line) and line.strip()]


def main():
    if len(sys.argv) != 4 or sys.argv[1] not in ('layout', 'symbol', 'codegen'):
        sys.exit('usage: gates.py {layout|symbol|codegen} C-artifact CXX-artifact')
    gate, left, right = sys.argv[1:]
    try:
        normalize = {'layout': layout, 'symbol': symbols, 'codegen': codegen}[gate]
        if gate == 'symbol':
            modes = renderer_mode(left), renderer_mode(right)
            if None not in modes and modes[0] != modes[1]:
                raise ValueError('renderer build modes differ')
            boundary = modes != (False, False)
            a, b = symbols(left, boundary), symbols(right, boundary)
        else:
            a, b = normalize(left), normalize(right)
        diff = ''.join(difflib.unified_diff(a, b, fromfile=left, tofile=right))
        print(f'{"FAIL" if diff else "PASS"}: {gate} {left} {right}')
        if diff:
            print(diff, end='')
        return bool(diff)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f'FAIL: {gate}: {error}')
        return 2


if __name__ == '__main__':
    sys.exit(main())
