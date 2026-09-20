#!/usr/bin/env python3
"""A timed-out loopback client must be reaped even when it ignores SIGTERM."""
import signal
import subprocess
import sys
import time

from netcode_runtime import stop_client

process = subprocess.Popen([sys.executable, '-c',
    'import signal,time; signal.signal(signal.SIGTERM, signal.SIG_IGN); '
    'print("ready",flush=True); time.sleep(30)'],
    stdout=subprocess.PIPE, text=True, start_new_session=True)
try:
    assert process.stdout.readline().strip() == 'ready'
    started = time.monotonic()
    stop_client(process, grace=0.1)
    assert process.returncode == -signal.SIGKILL, process.returncode
    assert time.monotonic() - started < 3
    stop_client(process, grace=0.1)  # Reaping an already-finished child is harmless.
finally:
    if process.poll() is None:
        process.kill()
        process.wait()
print('PASS: unresponsive loopback client is killed and reaped within the cleanup deadline')
