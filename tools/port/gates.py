#!/usr/bin/env python3
"""Compare C/C++ artifacts; preserve linkage, layout, and instruction differences."""
import difflib
import re
import subprocess
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
    if '.debug_info' not in run('readelf', '-S', '-W', path):
        raise ValueError(f'{path}: missing DWARF; compile with -g -fno-eliminate-unused-debug-types')
    raw = run('pahole', '--sort', '-a', '-A', '-I', '-M', path)
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


def symbols(path):
    rows = []
    for line in run('nm', '--defined-only', '--format=posix', path).splitlines():
        fields = line.split()
        name, kind = fields[:2]
        if name.startswith(('.L', '__func__.', '__FUNCTION__.', '__PRETTY_FUNCTION__.')) or kind in ('a', 'N'):
            continue  # compiler labels, function-name strings, debug/file metadata
        demangled = run('c++filt', name).strip()
        # Local statics have function scope in C++ demangling only.
        normalized = plain_symbol(demangled).split('::')[-1]
        if normalized in {
            'GetRefAPI', 'dllEntry', 'vmMain', 'snd_p', 'snd_out',
            'snd_linear_count', 'Q_setjmp_c', 'Q_longjmp_c', 'CPUID_EX',
            'Q_GetFPUCW', 'Q_SetFPUCW', 'NvOptimusEnablement',
            'AmdPowerXpressRequestHighPerformance',
        } or normalized.startswith('S_WriteLinearBlastStereo16_'):
            normalized = name  # External loaders/assembly require the C spelling.
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
