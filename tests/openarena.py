#!/usr/bin/env python3
"""Stage installed OpenArena data with the upstream GPL QVM release, outside git."""
import argparse
import hashlib
import io
from pathlib import Path
from run import SCRATCH
import urllib.request
import zipfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--data', type=Path, default=Path('/usr/share/games/openarena/baseoa'))
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-openarena-baseoa'))
args = parser.parse_args()
paks = sorted(args.data.resolve().glob('*.pk3'))
if not paks:
    parser.error('install openarena-data first; content is required')
args.output.mkdir(parents=True, exist_ok=True)
for pak in paks:
    target = args.output / pak.name
    if not target.exists():
        target.symlink_to(pak)
# Debian/Ubuntu replace upstream QVMs with native-module markers. Quake3e needs
# real QVMs for the pre-#2 oracle. Pin the official release, never a moving build.
url = 'https://github.com/OpenArena/gamecode/releases/download/oaxB52/oaxB52.zip'
with urllib.request.urlopen(url, timeout=60) as response:
    archive = response.read()
if hashlib.sha256(archive).hexdigest() != '91cb4e677d1a9f1741391ebc5fe06054ed25073addf050baf80cfc7550f98c94':
    raise SystemExit('FAIL: OpenArena gamecode archive checksum changed')
with zipfile.ZipFile(io.BytesIO(archive)) as package:
    (args.output / 'zz-oax.pk3').write_bytes(package.read('oaxB52/oax.pk3'))
    (args.output / 'COPYING-gamecode').write_bytes(package.read('oaxB52/COPYING'))
print('Prepared OpenArena content:', args.output)
