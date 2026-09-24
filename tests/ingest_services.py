#!/usr/bin/env python3
"""Run durable ingest contracts on a private, disposable real PostgreSQL database."""
import argparse
import json
import os
from pathlib import Path
import secrets
import shutil
import subprocess
import tempfile
import time
import uuid
from run import ROOT, SCRATCH

POSTGRES = 'postgres:18-bookworm@sha256:3725f4e2499eef5134592b3b4ab79a543ed7f8e533b05b5b637af926630f6650'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-ingest-services')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
assert shutil.which('docker'), 'Docker is required; persistence checks never silently skip'
name = 'aftershock-ingest-'+uuid.uuid4().hex[:12]
with tempfile.TemporaryDirectory(prefix='aftershock-ingest-db-', dir=SCRATCH) as temporary:
    config = Path(temporary)/'postgres.env'
    password = secrets.token_hex(32)
    config.write_text('POSTGRES_PASSWORD='+password+'\nPOSTGRES_DB=results_test\n')
    config.chmod(0o600)
    started = False
    try:
        with (args.output/'postgres-start.log').open('w') as output:
            subprocess.run(['docker', 'run', '-d', '--rm', '--name', name, '--env-file', str(config),
                            '-p', '127.0.0.1::5432', POSTGRES], check=True, stdout=output, stderr=subprocess.STDOUT)
        started = True
        deadline = time.monotonic()+45
        while subprocess.run(['docker', 'exec', name, 'pg_isready', '-U', 'postgres', '-d', 'results_test'],
                             stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
            assert time.monotonic() < deadline, 'private results database did not become ready'
            time.sleep(.2)
        ports = json.loads(subprocess.check_output(['docker', 'inspect', '--format', '{{json .NetworkSettings.Ports}}', name]))
        port = ports['5432/tcp'][0]['HostPort']
        environment = dict(os.environ, INGEST_TEST_DATABASE=f'postgres://postgres:{password}@127.0.0.1:{port}/results_test?sslmode=disable')
        with (args.output/'contracts.log').open('w') as output:
            subprocess.run(['go', 'test', '-race', '-tags', 'ingest_integration', '-run', '^TestIngest', '-count=1', './...'],
                           cwd=ROOT/'tools/match', env=environment, check=True, stdout=output, stderr=subprocess.STDOUT)
    finally:
        if started:
            subprocess.run(['docker', 'rm', '-f', '-v', name], check=True, stdout=subprocess.DEVNULL)
print('PASS: transactional ingest, retry/restart idempotency and read-only results on PostgreSQL')
