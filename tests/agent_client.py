#!/usr/bin/env python3
"""Keep JSON separate from logs when a headless wrapper merges stderr into stdout."""
import os
import io
import queue
from types import SimpleNamespace
from pathlib import Path
from run import SCRATCH
import sys
import tempfile
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.agent import Engine

with tempfile.TemporaryDirectory(prefix='aftershock-agent-client-', dir=os.environ.get('AFTERSHOCK_SCRATCH')) as temporary:
    root = Path(temporary)
    data = root / 'data'
    data.mkdir()
    # The stub never loads content; the launcher only needs a pak-shaped path.
    (data / 'stub.pk3').touch()
    binary = root / 'engine'
    binary.write_text('''#!/usr/bin/env python3
import json
import sys
print('ordinary startup log', file=sys.stderr, flush=True)
for line in sys.stdin:
    request = json.loads(line)
    print(json.dumps({'id': request['id'], 'ok': True, 'result': {'op': request['op']}}), flush=True)
''')
    binary.chmod(0o700)
    wrapper = root / 'xvfb-run'
    wrapper.write_text('#!/bin/sh\nshift\n"$@" 2>&1\n')
    wrapper.chmod(0o700)
    with patch.dict(os.environ, PATH=str(root) + os.pathsep + os.environ['PATH'], VK_DRIVER_FILES='unused-by-stub'):
        with Engine(binary, data) as engine:
            assert engine.request('hello') == {'op': 'hello'}
            assert 'ordinary startup log' in engine.log_path.read_text()
# Events must consume the request's total budget, not restart its timeout.
client = Engine.__new__(Engine)
client.sequence, client.events, client.replies = 0, [], queue.Queue()
client.process = SimpleNamespace(stdin=io.StringIO())
client.log_path = Path(__file__)
for event in ({'event': 'warning'}, {'event': 'warning'}, {'id': 1, 'ok': True, 'result': {}}):
    client.replies.put(event)
with patch('time.monotonic', side_effect=[0, 0.05, 0.10, 0.15]):
    try:
        client.request('slow', timeout=0.12)
        raise AssertionError('event stream reset the request deadline')
    except TimeoutError:
        pass
print('PASS: separate JSON/log pipes and a total request deadline across events')

