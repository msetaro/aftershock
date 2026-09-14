#!/usr/bin/env python3
"""Reproduce T25 cleanup metadata differences without editing the repository.

Usage: python3 reproduce_t25_cleanup.py /path/to/repo [OUTPUT_DIRECTORY]
Requires make, gcc/g++, clang/clang++, readelf. Uses the repository's actual
single-object debug recipes. All experimental source/object writes go to OUTPUT.
A successful exit means the documented hash conflict was reproduced, not that
cleanup passes the strict unchanged-object gate.
"""
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

repo = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
out = Path(sys.argv[2] if len(sys.argv) > 2 else tempfile.mkdtemp(prefix='port-review-t25-repro-')).resolve()
out.mkdir(parents=True, exist_ok=True)
source = (repo / 'code/server/sv_client.cpp').read_text()
old = '''#ifdef __cplusplus
		bGood = (qboolean)( bGood & FS_FileIsInPAK( "vm/ui.qvm", &nChkSum2, NULL ) );
#else
		bGood &= FS_FileIsInPAK( "vm/ui.qvm", &nChkSum2, NULL );
#endif'''
assert source.count(old) == 1, 'Expected exactly one original T25 block'
line = old.splitlines()[1]
variants = {'original': source, 'plain': source.replace(old, line),
            'blank': source.replace(old, '\n' + line + '\n\n\n')}
rows = []
for cc, cxx in [('gcc', 'g++'), ('clang', 'clang++')]:
    target = str(out / 'debug-linux-x86_64/ded/sv_client.o')
    make = ['make', '-Bn', 'V=1', 'CC=' + cc, 'CXX=' + cxx,
            'BUILD_DIR=' + str(out), target]
    recipe = subprocess.check_output(make, cwd=repo, text=True)
    (out / (cc + '.make-command')).write_text(shlex.join(make) + '\n')
    (out / (cc + '.make-output')).write_text(recipe)
    commands = [shlex.split(s) for s in recipe.splitlines()
                if ' -c code/server/sv_client.cpp' in s]
    args = next(a for a in commands if a[a.index('-o') + 1] == target)
    args = [a for a in args if a not in ('-MMD', '-MP')]
    args[args.index('code/server/sv_client.cpp')] = str(out / 'source.cpp')
    args[1:1] = ['-I' + str(repo / 'code/server')]
    args[args.index('-o') + 1] = str(out / 'current.o')
    (out / (cc + '.version')).write_text(subprocess.check_output([cxx, '--version'], text=True))
    for dwarf in ('default', 'dwarf4'):
        command = args + (['-gdwarf-4'] if dwarf == 'dwarf4' else [])
        for mode, text in variants.items():
            # Same source path AND output path for every run; filenames cannot
            # explain differences. Saved artifacts are copied after compilation.
            (out / 'source.cpp').write_text(text)
            name = cc + '-' + dwarf + '-' + mode
            (out / (name + '.command')).write_text(shlex.join(command) + '\n')
            result = subprocess.run(command, cwd=repo, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            (out / (name + '.log')).write_text(result.stdout)
            assert result.returncode == 0, name + ': see compile log'
            data = (out / 'current.o').read_bytes()
            (out / (name + '.o')).write_bytes(data)
            rows.append({'compiler': cc, 'dwarf': dwarf, 'variant': mode,
                         'sha256': hashlib.sha256(data).hexdigest(), 'size': len(data)})

a = (out / 'clang-default-original.o').read_bytes()
b = (out / 'clang-default-blank.o').read_bytes()
original_md5 = hashlib.md5(variants['original'].encode()).digest()
blank_md5 = hashlib.md5(variants['blank'].encode()).digest()
start = a.find(original_md5)
assert start >= 0 and a.find(original_md5, start + 1) == -1, 'Source MD5 must occur once'
assert a[:start] == b[:start] and a[start + 16:] == b[start + 16:]
assert b[start:start + 16] == blank_md5
for mode in ('original', 'blank'):
    raw = subprocess.check_output(['readelf', '--debug-dump=rawline',
                                  str(out / ('clang-default-' + mode + '.o'))], text=True)
    (out / ('clang-' + mode + '.rawline')).write_text(raw)
    assert 'source.cpp' in raw
hashes = {(r['compiler'], r['dwarf'], r['variant']): r['sha256'] for r in rows}
for cc in ('gcc', 'clang'):
    for dwarf in ('default', 'dwarf4'):
        assert hashes[cc, dwarf, 'original'] != hashes[cc, dwarf, 'plain']
        equal = hashes[cc, dwarf, 'original'] == hashes[cc, dwarf, 'blank']
        assert equal == (cc == 'gcc' or dwarf == 'dwarf4')
report = {'repo': str(repo), 'git_head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip(),
          'rows': rows, 'clang_default_md5': {'offset_start': start, 'offset_end_inclusive': start + 15,
          'original': original_md5.hex(), 'blank': blank_md5.hex(),
          'differing_byte_offsets': [i for i, (x, y) in enumerate(zip(a, b)) if x != y]}}
(out / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
print('REPRODUCED: GCC blank-line cleanup hashes match; Clang DWARF5 changes only source MD5.')
print('Clang source MD5 byte offsets:', start, 'through', start + 15)
print('DWARF4 is a diagnostic control, not a default-flags hash-gate pass.')
print('Evidence:', out / 'results.json')
