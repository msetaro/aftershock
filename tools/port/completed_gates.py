#!/usr/bin/env python3
"""Rebuild current native completed TUs and rerun G2/G3/G4 after header changes."""
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import subprocess
import sys

os.chdir(Path(__file__).resolve().parents[2])
output = Path(sys.argv[1] if len(sys.argv) > 1 else '/tmp/aftershock-cpp-port/final-gates').resolve()
output.mkdir(parents=True, exist_ok=True)
rows = re.findall(r'^\| `(code/[^`]+\.c)` \| done \| (.*)',
                  Path('docs/cpp-port-progress.md').read_text(), re.M)


def check(row):
    source, detail = row
    if Path(source).parts[1] == 'win32' or Path(source).name in ('vm_aarch64.c', 'vm_armv7l.c', 'vm_powerpc.c'):
        return dict(source=source, skipped='separate cross-target gate results')
    context = re.search(r'\(([^() ]+\.o)(?:,|\))', detail)
    if not context:
        return dict(source=source, skipped=detail)
    stem = Path(source).stem
    target = output / Path(source).parts[1] / stem
    target.mkdir(parents=True, exist_ok=True)
    variables = ['USE_SDL=0'] if 'nosdl' in detail else []
    if source == 'code/unix/linux_joystick.c':
        variables.append('CFLAGS=-DUSE_JOYSTICK')
    with (target / 'compile.log').open('w') as log:
        status = subprocess.run(['python3', 'tools/port/compile_pair.py', context[1], str(target),
                                 *variables], stdout=log, stderr=subprocess.STDOUT).returncode
    result = dict(source=source, object=context[1], variables=variables, compile=status)
    if status:
        return result
    for gate, suffix in [('layout', 'o'), ('symbol', 'sym.o'), ('codegen', 's')]:
        with (target / (gate + '.diff')).open('w') as log:
            result[gate] = subprocess.run([f'tools/port/{gate}_gate.sh',
                                          str(target / f'{stem}.c.{suffix}'),
                                          str(target / f'{stem}.cxx.{suffix}')],
                                         stdout=log, stderr=subprocess.STDOUT).returncode
    print(source, result, flush=True)
    return result


with ThreadPoolExecutor(8) as pool:
    results = list(pool.map(check, rows))
(output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
failed = [row for row in results if any(row.get(key, 0) for key in ('compile', 'layout', 'symbol'))]
skipped = [row for row in results if 'skipped' in row]
print(f'{"FAIL" if failed else "PASS"}: {len(results)-len(skipped)} native TUs, '
      f'{len(failed)} blocking failures, {len(skipped)} cross-target/include/utility skips; G4 advisory differences retained')
sys.exit(bool(failed))
