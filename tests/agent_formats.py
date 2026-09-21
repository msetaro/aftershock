#!/usr/bin/env python3
"""Check discoverable authored-format schemas and actionable validation errors."""
import copy
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from jsonschema import Draft202012Validator
from run import ROOT

invalid = {'level': ('version', 0), 'weapon': ('interval_ms', 0),
           'animation': ('states', []), 'material': ('alphaMode', 'invalid'),
           'effect': ('emitters', []), 'match-spec': ('players', 0)}
with tempfile.TemporaryDirectory(prefix='aftershock-formats-', dir=os.environ.get('AFTERSHOCK_SCRATCH')) as temporary:
    for kind, (key, value) in invalid.items():
        description = subprocess.run([sys.executable, 'tools/agent', 'describe', kind], cwd=ROOT,
                                     capture_output=True, text=True)
        assert description.returncode == 0, description.stderr
        contract = json.loads(description.stdout)
        Draft202012Validator.check_schema(contract['schema'])
        Draft202012Validator(contract['schema']).validate(contract['example'])
        source = Path(temporary)/(kind+'.json')
        source.write_text(json.dumps(contract['example']))
        command = [sys.executable, 'tools/agent', 'validate', kind, str(source)]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        assert result.returncode == 0, result.stderr
        changed = copy.deepcopy(contract['example'])
        changed[key] = value
        source.write_text(json.dumps(changed))
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        assert result.returncode == 1, result.stdout
        error = json.loads(result.stderr)['error']
        assert error['file'] == str(source) and key in error['path'] and error['hint'], error
print('PASS: six authored schemas, minimal examples and file/path/type/range diagnostics')
