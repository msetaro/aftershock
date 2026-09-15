#!/usr/bin/env python3
"""Check native alignment and payload preservation in OpenArena's real allocator."""
import argparse
import io
from pathlib import Path
import shlex
import subprocess
import tarfile

from run import ROOT, run

REVISION = '331464ca396d80e91cf9be273588f2b5f4b7afc8'
REPOSITORY = 'https://github.com/OpenArena/gamecode'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='gcc')
parser.add_argument('--source', type=Path, default=Path('/tmp/aftershock-oa-native-source'))
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-openarena-alloc'))
args = parser.parse_args()
source, output = args.source.resolve(), args.output.resolve()
if not source.exists():
    run(['git', 'init', source])
    run(['git', '-C', source, 'fetch', '--depth=1', REPOSITORY, REVISION])
archive = subprocess.check_output(['git', '-C', source, 'archive', REVISION, 'code/game', 'code/qcommon'])
output.mkdir(parents=True, exist_ok=True)
with tarfile.open(fileobj=io.BytesIO(archive)) as package:
    package.extractall(output, filter='data')
patch = ROOT / 'tests/patches/openarena-allocation-alignment.patch'
if patch.exists():
    subprocess.run(['git', 'apply', str(patch)], cwd=output, check=True)
binary = output / 'check'
run([*shlex.split(args.cc), '-std=gnu99', '-O2', '-fno-builtin',
     '-ffunction-sections', '-fdata-sections', '-fsanitize=undefined,address',
     '-fno-sanitize-recover=all', '-I' + str(output / 'code/game'),
     output / 'code/game/bg_alloc.c', 'tests/probes/openarena_alloc.c',
     '-Wl,--gc-sections', '-o', binary])
run([binary])
print('PASS: OpenArena allocator aligns native payloads and preserves live allocations')
