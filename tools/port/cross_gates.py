#!/usr/bin/env python3
"""Recheck built, unblocked engine objects using cross or native C manifests."""
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import subprocess
import sys

os.chdir(Path(__file__).resolve().parents[2])
output = Path(sys.argv[1] if len(sys.argv) > 1 else '/tmp/aftershock-cpp-port/cross-gates').resolve()
output.mkdir(parents=True, exist_ok=True)
progress = Path('docs/cpp-port-progress.md').read_text()
sources = re.findall(r'^\| `(code/[^`]+\.c(?:pp)?)` \| done \|', progress, re.M)
stems = {Path(source).stem for source in sources}
tasks = []
evidence = Path(os.environ.get('PORT_EVIDENCE', 'tools/port/evidence')).resolve()
if not (evidence / 'cross-oracles.json').is_file():
    sys.exit('Set PORT_EVIDENCE to the extracted port-evidence archive; see docs/cpp-port-progress.md')
targets = json.loads((evidence / 'cross-oracles.json').read_text())
if len(sys.argv) > 2 and sys.argv[2] == 'native':
    targets = {'native': {'variables': [], 'manifest': 'phase0-c.sha256'},
               'native-nosdl': {'variables': ['USE_SDL=0'], 'manifest': 'nosdl-c.sha256'}}
for name, target in targets.items():
    suffix = '-nolto-nosdl' if name == 'mingw64' else ''
    variables = target['variables'] + (['USE_SDL=0', 'OPTIMIZE=-O2 -ffast-math -fno-lto'] if suffix else [])
    manifest = evidence / target.get('manifest', f'cross-{name}{suffix}-c.sha256')
    for line in manifest.read_text().splitlines():
        _, path = line.split()
        if Path(path).stem in stems:
            tasks.append((name, path.split('/', 1)[1], variables))

if not tasks:
    sys.exit('FAIL: no engine objects matched the evidence manifests and progress inventory')


def check(task):
    name, obj, variables = task
    if obj == 'client/linux_joystick.o':
        # The default object is empty; check its actual body as in completed_gates.
        variables = [*variables, 'CFLAGS=-DUSE_JOYSTICK']
    target = output / name / obj.removesuffix('.o')
    target.mkdir(parents=True, exist_ok=True)
    stem = Path(obj).stem
    with (target / 'compile.log').open('w') as log:
        status = subprocess.run(['python3', 'tools/port/compile_pair.py', obj, str(target), *variables],
                                stdout=log, stderr=subprocess.STDOUT).returncode
    result = dict(target=name, object=obj, variables=variables, compile=status)
    if not status:
        for gate, ext in [('layout', 'o'), ('symbol', 'sym.o'), ('codegen', 's')]:
            with (target / (gate + '.diff')).open('w') as log:
                result[gate] = subprocess.run([f'tools/port/{gate}_gate.sh',
                                              str(target / f'{stem}.c.{ext}'),
                                              str(target / f'{stem}.cxx.{ext}')],
                                             stdout=log, stderr=subprocess.STDOUT).returncode
    print(result, flush=True)
    return result


with ThreadPoolExecutor(4) as pool:
    results = list(pool.map(check, tasks))
(output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
failed = [row for row in results if any(row.get(key, 0) for key in ('compile', 'layout', 'symbol'))]
print(f'{"FAIL" if failed else "PASS"}: {len(results)} objects, {len(failed)} blocking failures; G4 advisory')
sys.exit(bool(failed))
