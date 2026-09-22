#!/usr/bin/env python3
"""Verify structured development commands and replies through the native dispatcher."""
import argparse
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import resource
import tempfile
from run import ROOT, run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx', default='g++')
args = parser.parse_args()
scratch = os.environ.get('AFTERSHOCK_SCRATCH')
if scratch:
    Path(scratch).mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-agent-protocol-', dir=scratch) as temporary:
    binary = Path(temporary)/'protocol'
    run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
         '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
         '-DAFTERSHOCK_DEVTOOLS', '-DUSE_VULKAN_API', '-ffunction-sections', '-fdata-sections',
         'tests/probes/agent_protocol.cpp', 'engine/qcommon/q_shared.cpp',
         '-Wl,--gc-sections', '-o', binary])
    result = run([binary], capture_output=True, timeout=10)
    replies = [json.loads(line) for line in result.stdout.splitlines()]
    assert [row['id'] for row in replies] == list(range(1, 15))
    assert replies[0]['ok'] and replies[0]['result']['protocol'] == 1
    assert {'hello', 'exec', 'cvar.get', 'cvar.set'} <= set(replies[0]['result']['commands'])
    assert replies[1]['result'] == {'name': 'example', 'value': 'initial'}
    assert replies[2]['ok'] and replies[3]['result']['value'] == 'quote " slash \\ newline\n'
    assert replies[5]['ok']
    assert 'trace' in replies[0]['result']['commands']
    assert replies[9]['ok'] and replies[9]['result'] == dict(fraction=.25,end=[2,3,4],normal=[0,0,1],
        start_solid=False,all_solid=False,contents=1,surface_flags=0)
    for index, code, path in ((4, 'read_only', '$.name'), (6, 'unknown_operation', '$.op'),
                              (7, 'invalid_argument', '$.value'), (8, 'not_found', '$.name')):
        reply = replies[index]
        assert not reply['ok'] and reply['error']['code'] == code
        assert reply['error']['path'] == path and reply['error']['hint']
    profile = replies[10]['result']
    assert profile['frame'] == dict(serial=7, milliseconds=50, dropped=1)
    assert profile['cpu'] == [dict(name='frame', milliseconds=50, self_ms=30, parent=None),
                              dict(name='commands', milliseconds=20, self_ms=20, parent=0)]
    assert profile['history'] == [dict(serial=7, milliseconds=50)]
    assert replies[11]['error']['code'] == replies[12]['error']['code'] == 'invalid_argument'
    assert replies[13]['ok'] and replies[13]['result']['frame'] is None
    assert replies[13]['result']['cpu'] == replies[13]['result']['history'] == []
    resource.setrlimit(resource.RLIMIT_CORE, (0, resource.getrlimit(resource.RLIMIT_CORE)[1]))
    failed = subprocess.run([binary, '--assert'], cwd=temporary, capture_output=True, timeout=10)
    assert failed.returncode == -signal.SIGABRT and b'agent assertion contract' in failed.stderr
    events = [json.loads(line) for line in failed.stdout.splitlines()]
    assert len(events) == 1 and events[0]['event'] == 'assert', events
    assert 'agent assertion contract' in events[0]['detail'] and events[0]['value'] > 0
    crowded = subprocess.run([binary, '--assert', '--full-queue'], cwd=temporary, capture_output=True, timeout=10)
    assert crowded.returncode == -signal.SIGABRT
    crowded_events = [json.loads(line) for line in crowded.stdout.splitlines()]
    assert crowded_events[-1]['event'] == 'assert', 'full event queue lost the fatal assertion'
    assert sum(row.get('dropped', 0) for row in crowded_events) == 1
    error = subprocess.run([binary, '--error', '--full-queue'], cwd=temporary, capture_output=True, timeout=10)
    assert error.returncode == 0
    error_events = [json.loads(line) for line in error.stdout.splitlines()]
    assert error_events[-1]['event'] == 'error', 'full event queue lost the engine error'
    assert sum(row.get('dropped', 0) for row in error_events) == 1
    source = Path(temporary) / 'release.cpp'
    bodies = ['#include <assert.h>\n#define Q_ASSERT assert\n',
              '#include "' + str(ROOT / 'engine/public/assert_public.h') + '"\n']
    objects = []
    for index, header in enumerate(bodies):
        source.write_text(header + 'int sample(int value) { Q_ASSERT(value > 0); return value + 1; }\n')
        output = Path(temporary) / f'release-{index}.o'
        run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-DNDEBUG', '-DAFTERSHOCK_DEVTOOLS',
             '-c', source, '-o', output])
        objects.append(output.read_bytes())
    assert objects[0] == objects[1], 'release assertion code changed'
print('PASS: native JSON contract, assertion event before abort and identical release assertion object')
