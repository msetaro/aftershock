#!/usr/bin/env python3
"""Keep the local full-suite command catalog tied to every active CI test job."""
import json
import subprocess
import sys
from pathlib import Path
from types import SimpleNamespace
from suite import local_command
from run import ROOT

result = subprocess.run([sys.executable,'tests/suite.py','--list'],cwd=ROOT,capture_output=True,text=True)
assert result.returncode == 0, result.stderr
jobs = json.loads(result.stdout)
assert len(jobs) == 10, [row['id'] for row in jobs]
assert {row['job'] for row in jobs} == {'format','tidy','lifetimes','unit','sanitizers','runtime','cross','match-server'}
assert len({row['id'] for row in jobs}) == len(jobs)
for row in jobs:
    assert row['steps'] and all(step['command'] for step in row['steps'])
    for step in row['steps']:
        subprocess.run(['bash','-n','-c',step['command']],check=True)
    assert not any('sudo apt-get' in step['command'] or '/bin/pip install' in step['command'] for step in row['steps'])
commands = '\n'.join(step['command'] for row in jobs for step in row['steps'])
for test in ('agent_formats','agent_cli','check_lifetimes','check_tidy','native_abi','demo','match_kind','protocol_runtime'):
    assert 'tests/'+test+'.py' in commands, test
assert 'clang++ -stdlib=libc++' in commands and 'mingw64.cmake' in commands and 'aarch64-linux.cmake' in commands
probe = local_command("${{ env.AFTERSHOCK_SCRATCH }}/aftershock-cook-python/bin/python -c 'import sys; print(sys.executable)'",
                      {}, SimpleNamespace(glslang=Path('glslang'), openarena_data=None))
executed = subprocess.run(['bash', '-e', '-c', probe], capture_output=True, text=True)
assert executed.returncode == 0 and executed.stdout.strip() == sys.executable, (probe, executed.stderr)
print('PASS: all ten active job variants, shell syntax and selected Python execution')

