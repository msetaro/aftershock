#!/usr/bin/env python3
"""Keep JSON separate from logs when a headless wrapper merges stderr into stdout."""
import os
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
print('PASS: headless launcher preserves JSON stdout and separate engine stderr')
