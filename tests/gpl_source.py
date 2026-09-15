"""Pinned original GPL headers for standalone native-game bug checks."""
from pathlib import Path
import subprocess

from run import run

REVISION = 'dbe4ddb10315479fc00086f08e25d968b4b43c49'


def source_headers(source):
    source = Path(source).resolve()
    if not source.exists():
        run(['git', 'clone', '--no-checkout', 'https://github.com/id-Software/Quake-III-Arena', source])
        run(['git', '-C', source, 'checkout', '--detach', REVISION])
    revision = run(['git', '-C', source, 'rev-parse', 'HEAD'], stdout=subprocess.PIPE).stdout.decode().strip()
    if revision != REVISION:
        raise SystemExit('FAIL: unexpected GPL header revision: ' + revision)
    run(['git', '-C', source, 'diff', '--quiet', REVISION, '--',
         'code/game/*.h', 'code/botlib/*.h', 'code/qcommon/*.h'])
    return source / 'code/game'
