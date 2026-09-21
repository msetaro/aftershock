#!/usr/bin/env python3
"""Compare native C/C++ layouts and symbols; retain advisory assembly diffs."""
import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import difflib
import hashlib
import json
from pathlib import Path
from run import SCRATCH
import re
import shlex
import subprocess
import sys

from run import ROOT, ENV

sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / 'tools/port'))
import gates

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-native-gates'))
parser.add_argument('--jobs', type=int, default=4)
parser.add_argument('--tidy', action='store_true', help='also retain the focused G7 diagnostics')
args = parser.parse_args()
if args.jobs < 1:
    parser.error('--jobs must be positive')
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
manifest = json.loads((ROOT / 'docs/native-game-import.json').read_text())
metadata = {
    'revision': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
    'compilers': {cc: subprocess.check_output([cc, '--version'], text=True) for cc in ('gcc', 'g++')},
    'sha256': {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
               for name in [*[row['path'] for row in manifest['files']], 'game/bg/native_abi_public.h']},
}
(output / 'metadata.json').write_text(json.dumps(metadata, indent=2) + '\n')


def check(item):
    module, source = item
    name = module + '-' + Path(source).stem
    folder = output / name
    folder.mkdir(exist_ok=True)
    flags = ['-O2', '-fPIC', '-ffp-contract=off', '-fno-strict-aliasing',
             '-fwrapv', '-fno-builtin', '-include', 'game/bg/native_abi_public.h',
             '-D' + {'game': 'QAGAME', 'cgame': 'CGAME', 'ui': 'UI'}[module]]
    # Existing port symbol-oracle flags, including header optimization isolation.
    symbol_flags = ['-g0', '-fno-inline-functions', '-D__NO_CTYPE=1',
                    '-U__OPTIMIZE__', '-U_FORTIFY_SOURCE', '-fno-inline-small-functions',
                    '-fno-inline-functions-called-once', '-fno-ipa-sra']
    cpp_mode = ['-x', 'c++', '-std=c++20', '-fno-exceptions', '-fno-rtti',
                '-U_GNU_SOURCE', '-D_DEFAULT_SOURCE']
    for language, compiler, mode in [
        ('c', 'gcc', ['-x', 'c', '-std=gnu99']),
        ('cpp', 'g++', cpp_mode)
    ]:
        for suffix, extra in [
            ('o', ['-c', '-g', '-fno-eliminate-unused-debug-types']),
            ('sym.o', ['-c', *symbol_flags]),
            ('s', ['-S', '-g0'])
        ]:
            target = folder / (language + '.' + suffix)
            command = [compiler, *mode, *flags, *extra, source, '-o', str(target)]
            target.with_suffix(target.suffix + '.command').write_text(shlex.join(command) + '\n')
            with target.with_suffix(target.suffix + '.log').open('w') as log:
                subprocess.run(command, cwd=ROOT, env=ENV, stdout=log,
                               stderr=subprocess.STDOUT, check=True)
    result = {}
    for gate, suffix, normalize in [('layout', 'o', gates.layout),
                                    ('symbol', 'sym.o', gates.symbols),
                                    ('codegen', 's', gates.codegen)]:
        before, after = [normalize(str(folder / (language + '.' + suffix)))
                         for language in ('c', 'cpp')]
        (folder / (gate + '.diff')).write_text(''.join(difflib.unified_diff(
            before, after, fromfile='C', tofile='C++')))
        result[gate] = before == after
    if args.tidy:
        command = ['clang-tidy', source, '--quiet',
                   '--checks=-*,bugprone-signed-char-misuse,bugprone-narrowing-conversions,bugprone-suspicious-string-compare',
                   '--', *cpp_mode, *flags]
        if source == 'game/bg/bg_lib.cpp':
            command += ['-D__NO_INLINE__']
        (folder / 'tidy.command').write_text(shlex.join(command) + '\n')
        analysis = subprocess.run(command, cwd=ROOT, env=ENV, text=True,
                                  stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (folder / 'tidy.log').write_text(analysis.stdout)
        analysis.check_returncode()
        result['tidy'] = dict(Counter(re.findall(r'warning:.*\[([^]]+)\]', analysis.stdout)))
    return name, result


tasks = [(module, source) for module, sources in manifest['modules'].items()
         for source in [*sources, 'game/bg/bg_lib.cpp']]
with ThreadPoolExecutor(max_workers=args.jobs) as pool:
    results = dict(pool.map(check, tasks))
(output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
for gate in ('layout', 'symbol', 'codegen'):
    differences = [name for name, row in results.items() if not row[gate]]
    print(gate, 'differences:', differences)
print(len(results), 'objects; codegen differences require advisory review')
if args.tidy:
    findings = Counter()
    for row in results.values():
        findings.update(row['tidy'])
    print('tidy findings requiring disposition:', dict(findings))
if any(not row['layout'] or not row['symbol'] for row in results.values()):
    raise SystemExit(1)
