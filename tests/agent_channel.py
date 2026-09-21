#!/usr/bin/env python3
"""Check local NDJSON transport and engine-owned deterministic stepping."""
import argparse
import json
import os
from pathlib import Path
import selectors
import subprocess
import tempfile
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True, help='development dedicated server')
parser.add_argument('--data', type=Path, default=Path.home()/'.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
args = parser.parse_args()
scratch = os.environ.get('AFTERSHOCK_SCRATCH')
if scratch:
    Path(scratch).mkdir(parents=True, exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
assert paks, 'installed game content is required'
with tempfile.TemporaryDirectory(prefix='aftershock-agent-channel-', dir=scratch) as temporary:
    home = Path(temporary)
    game = 'baseoa' if args.content == 'openarena' else 'baseq3'
    (home/game).mkdir()
    for pak in paks:
        (home/game/pak.name).symlink_to(pak)
    with (home/'engine.log').open('wb') as log:
        process = subprocess.Popen([str(args.binary.resolve()), '--agent',
                                    '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home),
                                    '+set', 'fs_game', game, '+set', 'net_enabled', '0',
                                    '+set', 'sv_pure', '0'], stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE, stderr=log, bufsize=0)
        try:
            sequence = 0
            pending = bytearray()
            with selectors.DefaultSelector() as selector:
                selector.register(process.stdout, selectors.EVENT_READ)
                def request(op, **fields):
                    global sequence
                    sequence += 1
                    process.stdin.write((json.dumps(dict(id=sequence, op=op, **fields))+'\n').encode())
                    deadline = time.monotonic()+20
                    while b'\n' not in pending:
                        assert time.monotonic() < deadline, f'{op}: no JSON reply; '+(home/'engine.log').read_text(errors='replace')[-2000:]
                        if selector.select(timeout=.1):
                            data = os.read(process.stdout.fileno(), 65536)
                            assert data, f'{op}: engine exited {process.poll()}'
                            pending.extend(data)
                    line, _, rest = pending.partition(b'\n')
                    pending[:] = rest
                    reply = json.loads(line)
                    assert reply['id'] == sequence and reply['ok'], reply
                    return reply['result']
                assert request('hello')['protocol'] == 1
                assert request('session', dt=8, seed=123)['seed'] == 123
                first = request('step', frames=3)
                assert first == {'frame': 3, 'time': 1024, 'dt': 8}, first
                time.sleep(.15)  # Wall time must not advance the engine clock.
                second = request('step', frames=2)
                assert second == {'frame': 5, 'time': 1040, 'dt': 8}, second
                process.stdin.close()
                assert process.wait(timeout=20) == 0
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
print('PASS: local NDJSON, seeded fixed-dt frame stepping, idle clock and clean pipe shutdown')
