#!/usr/bin/env python3
"""Verify approved T25 cleanup: raw release and stripped-debug GCC/Clang hashes.

Uses the pre-cleanup source at 629700fa and the current source, compiled at the
same temporary pathname with actual Make flags; never edits the worktree.
"""
from pathlib import Path
import hashlib
import json
import os
import shlex
import subprocess
import sys
import tempfile

repo = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
out = Path(sys.argv[2] if len(sys.argv) > 2 else tempfile.mkdtemp(prefix='port-t25-')).resolve()
out.mkdir(parents=True, exist_ok=True)
source = 'code/server/sv_client.cpp'
before = subprocess.check_output(['git', 'show', '629700fa:' + source], cwd=repo)
after = (repo / source).read_bytes()
block = b'#ifdef __cplusplus\n\t\tbGood = (qboolean)( bGood & FS_FileIsInPAK( "vm/ui.qvm", &nChkSum2, NULL ) );\n#else\n\t\tbGood &= FS_FileIsInPAK( "vm/ui.qvm", &nChkSum2, NULL );\n#endif'
assert before.count(block) == 1 and before.replace(block, block.splitlines()[1]) == after
rows = []
env = dict(os.environ, SOURCE_DATE_EPOCH='1789257600', LC_ALL='C')
for cc, cxx in [('gcc', 'g++'), ('clang', 'clang++')]:
    for config in ['release', 'debug']:
        for context in ['client', 'ded']:
            target = out / f'{config}-linux-x86_64/{context}/sv_client.o'
            make = ['make', '-Bn', 'V=1', 'CC=' + cc, 'CXX=' + cxx, 'BUILD_DIR=' + str(out), str(target)]
            recipe = subprocess.check_output(make, cwd=repo, env=env, text=True)
            commands = [shlex.split(line) for line in recipe.splitlines() if ' -c ' + source in line]
            args = next(a for a in commands if a[a.index('-o') + 1] == str(target))
            args = [a for a in args if a not in ('-MMD', '-MP')]
            args[args.index(source)] = str(out / 'source.cpp')
            args[args.index('-o') + 1] = str(out / 'current.o')
            args[1:1] = ['-I' + str(repo / 'code/server')]
            row = dict(compiler=cc, config=config, context=context, command=args, cwd=str(repo))
            for tag, data in [('before', before), ('after', after)]:
                (out / 'source.cpp').write_bytes(data)
                subprocess.run(args, cwd=repo, env=env, check=True)
                row[tag + '_raw'] = hashlib.sha256((out / 'current.o').read_bytes()).hexdigest()
                subprocess.run(['objcopy', '--strip-debug', str(out / 'current.o'), str(out / 'stripped.o')], check=True)
                row[tag + '_stripped'] = hashlib.sha256((out / 'stripped.o').read_bytes()).hexdigest()
            key = 'raw' if config == 'release' else 'stripped'
            assert row['before_' + key] == row['after_' + key], row
            rows.append(row)
(out / 'results.json').write_text(json.dumps(rows, indent=2) + '\n')
print('PASS: 8 GCC/Clang client/ded release and stripped-debug object comparisons')
