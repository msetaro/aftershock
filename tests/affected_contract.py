#!/usr/bin/env python3
"""Check conservative path selection and the fast-feedback budget boundary."""
import importlib.util
import subprocess
import sys
import tempfile
from pathlib import Path
from run import ROOT

spec = importlib.util.spec_from_file_location('affected', ROOT/'tests/affected.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
assert 'physics' in module.select(['engine/physics/physics.cpp'])[0]
assert 'physics' in module.select(['third_party/joltc/src/joltc.cpp'])[0]
assert 'weapons' in module.select(['engine/weapons/weapons.cpp'])[0]
assert 'animation' in module.select(['tools/cook/animation.py'])[0]
assert 'agent_protocol' in module.select(['engine/devtools/dev_agent.cpp'])[0]
assert module.select(['unmapped/new.cpp'])[1] == ['unmapped/new.cpp']
assert 'unit' in module.select(['unmapped/new.cpp'])[0]
assert module.select(['engine/weapons/a.cpp','engine/weapons/b.cpp'])[0].count('weapons') == 1
assert 'affected_contract' in module.select(['tests/affected.py'])[0]
result = subprocess.run([sys.executable, 'tests/affected.py', 'HEAD', '--list'],cwd=ROOT,capture_output=True,text=True)
assert result.returncode == 0, result.stderr
import json
plan = json.loads(result.stdout)
assert plan['full'] is False and plan['budget_seconds'] == 600 and isinstance(plan['paths'],list)
with tempfile.TemporaryDirectory(prefix='affected-budget-') as directory:
    log = Path(directory)/'command.log'
    assert module.execute([sys.executable,'-c','raise SystemExit(3)'], log, 5)['status'] == 'failed'
    assert module.execute([sys.executable,'-c','import time; time.sleep(20)'], log, 0.1)['status'] == 'timeout'
print('PASS: affected paths, conservative fallback, deduplication and budget timeout')
