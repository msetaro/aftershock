#!/usr/bin/env python3
"""Offline contract for the repository-local immutable build publisher."""
import json
import os
from pathlib import Path
from run import SCRATCH
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SHA = '1234567890abcdef1234567890abcdef12345678'
with tempfile.TemporaryDirectory(prefix='aftershock-publish-') as temporary:
    folder = Path(temporary)
    fake = folder/'gh'
    fake.write_text('''#!/usr/bin/env python3
import json,os,sys
from pathlib import Path
args=sys.argv[1:]
with Path(os.environ['CALLS']).open('a') as stream: stream.write(json.dumps(args)+'\\n')
command=' '.join(args[:2])
if os.environ.get('FAIL')==command: sys.exit(2)
if args[0]=='api': print(os.environ.get('TAG_SHA',''))
elif command=='release view': sys.exit(0 if os.environ.get('RELEASE') else 1)
''')
    fake.chmod(0o755)
    artifact = folder/'owned-build.zip'
    artifact.write_bytes(b'owned test archive')
    calls = folder/'calls.jsonl'

    def publish(extra=None, succeeds=True):
        calls.write_text('')
        env = dict(os.environ, PATH=str(folder)+os.pathsep+os.environ['PATH'],
                   GITHUB_SHA=SHA, GITHUB_REPOSITORY='msetaro/aftershock', CALLS=str(calls),
                   TAG_SHA='', RELEASE='', FAIL='')
        env.update(extra or {})
        result = subprocess.run(['bash', str(ROOT/'tools/publish_build.sh'), str(artifact)],
                                env=env, text=True, capture_output=True)
        assert (result.returncode == 0) == succeeds, result.stderr
        return [json.loads(line) for line in calls.read_text().splitlines()]

    initial = publish()
    create = next(row for row in initial if row[:2] == ['release','create'])
    assert create[2] == 'build-'+SHA and create[create.index('--target')+1] == SHA
    assert create[create.index('--repo')+1] == 'msetaro/aftershock'
    assert str(artifact) in create
    retry = publish({'TAG_SHA':SHA, 'RELEASE':'1'})
    assert any(row[:2] == ['release','upload'] for row in retry)
    assert not any(row[:2] == ['release','create'] for row in retry)
    partial = publish({'TAG_SHA':SHA})
    assert any(row[:2] == ['release','create'] for row in partial)
    mismatch = publish({'TAG_SHA':'f'*40, 'RELEASE':'1'}, succeeds=False)
    assert all(row[0] == 'api' for row in mismatch)
    for failure in ('release create','release upload'):
        publish({'TAG_SHA':SHA, 'RELEASE':'1' if failure.endswith('upload') else '', 'FAIL':failure}, succeeds=False)
    failed_lookup = publish({'FAIL':'api repos/msetaro/aftershock/git/matching-refs/tags/build-'+SHA}, succeeds=False)
    assert len(failed_lookup) == 1
    assert publish({'GITHUB_REPOSITORY':'other/project'}, succeeds=False) == []
    assert publish({'GITHUB_SHA':'known-good-2026-09-20'}, succeeds=False) == []
    # This contract never calls the real gh or writes a remote ref/release.
    print('PASS: immutable local-repository publication, retry and failure controls')
