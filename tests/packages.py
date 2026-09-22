#!/usr/bin/env python3
"""Package owned cooked content and apply a one-texture manifest delta."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
from PIL import Image
from cook import cook, source_assets
from run import ROOT, SCRATCH

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=SCRATCH/'aftershock-packages')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
source = args.output/'source'
source.mkdir(exist_ok=True)
project, _ = source_assets(source)
definition = json.loads(project.read_text())
shutil.copyfile(source/'color.png', source/'patch.png')
for asset in definition['assets']:
    if asset['name'] == 'textures/bc7':
        asset['source'] = 'patch.png'
project.write_text(json.dumps(definition))
cooked = args.output/'cooked'
cook(project, cooked)
(cooked/'obsolete.cfg').write_text('set package_example 1\n')


def package(*arguments):
    command = [sys.executable, 'tools/package.py', *map(str, arguments)]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return json.loads(result.stdout)


def resources(directory):
    return {p.relative_to(directory).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in directory.rglob('*') if p.is_file() and not p.name.endswith('.manifest.json')}


base = args.output/'base.aspack'
first = package('build', '--root', cooked, '--output', base)
assert first['assets'] == resources(cooked)
assert first['base'] is None and len(first['identity']) == 64
repeat = args.output/'repeat.aspack'
package('build', '--root', cooked, '--output', repeat)
assert base.read_bytes() == repeat.read_bytes(), 'unchanged content must package reproducibly'

# One source edit changes one native texture plus the cook index/revision.
Image.new('RGBA', (16, 16), (255, 48, 16, 255)).save(source/'patch.png')
result = cook(project, cooked)
assert result['built'] == ['textures/bc7']
patch = args.output/'texture-patch.aspack'
second = package('build', '--root', cooked, '--output', patch, '--base', base)
assert second['base'] == first['identity']
assert set(second['assets']) == {'textures/bc7.ktx2', 'cook.index', 'cook.revision'}
assert not second['removed']
assert patch.stat().st_size < base.stat().st_size//2, 'one-texture patch must omit unchanged payloads'

restored = args.output/'restored'
package('extract', base, patch, '--output', restored)
assert resources(restored) == resources(cooked), 'mounted base plus patch must equal the recooked content'
# Removal is part of a manifest diff, not an old asset silently leaking through.
(cooked/'obsolete.cfg').unlink()
removed = args.output/'removed.aspack'
third = package('build', '--root', cooked, '--output', removed, '--base', base, '--base', patch)
assert third['removed'] == ['obsolete.cfg']
assert not third['assets']
final = args.output/'final'
package('extract', base, patch, removed, '--output', final)
assert resources(final) == resources(cooked)
print('PASS: one reproducible cooked pack, small one-texture delta, exact layered view and explicit removal')
