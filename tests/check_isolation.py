#!/usr/bin/env python3
"""Reject shared temporary paths in active tools, tests and workflows."""
from pathlib import Path
import subprocess
import tempfile
from run import ROOT


def violations(path):
    return [number for number,line in enumerate(path.read_text().splitlines(),1) if '/'+'tmp/' in line]


paths = subprocess.check_output(['git','ls-files','-z','tests','tools','.github'],cwd=ROOT).decode().split('\0')
failures = []
for name in paths:
    path = Path(name)
    # Historical/excluded drivers are unchanged and are not part of this gate.
    if not name or path.suffix not in ('.py','.sh','.yml','.yaml','.c','.cpp','.h') or 'fuzz' in path.parts or name=='tests/network.py' or name.startswith('tools/port/'):
        continue
    failures.extend(f'{name}:{line}' for line in violations(ROOT/path))
with tempfile.TemporaryDirectory(prefix='aftershock-isolation-policy-') as temporary:
    control = Path(temporary)/'control.py'
    control.write_text("output = '"+'/'+'tmp/shared'+"'\n")
    assert violations(control)==[1], 'fixed-path negative control escaped the gate'
    control.write_text("output = SCRATCH / 'private'\n")
    assert not violations(control)
if failures:
    raise SystemExit('FAIL: shared temporary paths: '+', '.join(failures))
print('PASS: active test/tool paths use invocation roots; negative control rejects fixed paths')
