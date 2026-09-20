#!/usr/bin/env python3
"""Exercise local entity edit/save/reload while preserving the map's unknown keys."""
import argparse
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
import zipfile
from run import ROOT, build, content_maps, content_settings

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path)
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-dev-entities'))
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
# Entity editing belongs to this repository's native game, also when using OA art.
if not args.binary:
    args.binary = build(args.output / 'build', ['BUILD_SERVER=0', 'AFTERSHOCK_DEVTOOLS=1']) / 'quake3e.x64'
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if not paks or len(icds) != 1:
    parser.error('installed content and one lavapipe ICD are required')
map_name = content_maps(args.content)[0]
original = None
for pak in paks:
    with zipfile.ZipFile(pak) as archive:
        if f'maps/{map_name}.bsp' in archive.namelist():
            bsp = archive.read(f'maps/{map_name}.bsp')
            offset, length = struct.unpack_from('<ii', bsp, 8)
            original = bsp[offset:offset + length].rstrip(b'\0')
assert original is not None

def tokens(text):
    # Trusted installed map text; compare quoted tokens, preserving backslashes.
    return re.findall(rb'"[^"]*"|[{}]', text)

with tempfile.TemporaryDirectory(prefix='aftershock-entity-edit-') as temp:
    home = Path(temp)
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    script = [f'devmap {map_name}', 'wait 10', 'dev_world collision 2048',
              'dev_world nav 2048', 'dev_world both 2048', 'dev_entity save',
              'dev_entity spawn target_position 1 2 128',
              'dev_entity set last targetname dev_tools_entity_test',
              'dev_entity set last count 7', 'dev_entity set last origin "4 5 129"',
              'dev_entity get last count', 'dev_entity save',
              'dev_entity delete last', 'dev_entity save',
              f'set dev_entityFile maps/{map_name}.dev.001.ent',
              'set dev_loadEntities 1', 'map_restart 0', 'wait 10',
              'dev_entity find dev_tools_entity_test', 'dev_entity get last count',
              'dev_entity get last origin', 'dev_entity save', 'quit']
    (base / 'entity-check.cfg').write_text('\n'.join(script) + '\n')
    command = ['timeout', '90', 'xvfb-run', '-a', str(args.binary.resolve()),
               '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home),
               *content_settings(args.content), '+set', 'net_enabled', '0',
               '+set', 'sv_pure', '0', '+set', 's_initsound', '0', '+set', 'r_fullscreen', '0',
               '+set', 'r_mode', '3', '+set', 'fixedtime', '50', '+set', 'com_maxfps', '0', '+set', 'cl_autoRecordDemo', '0',
               '+exec', 'entity-check.cfg']
    env = dict(os.environ, VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]), LP_NUM_THREADS='1')
    result = subprocess.run(command, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (args.output / 'client.log').write_bytes(result.stdout)
    result.check_returncode()
    text = result.stdout.decode(errors='replace')
    assert not any(error in text for error in ('ERROR:', 'Signal caught', 'Unknown command', 'rejected', 'write failed'))
    world = re.findall(r'Developer world: (\d+) lines, (\d+) omitted', text)
    assert len(world) == 3 and all(int(row[0]) > 0 for row in world), 'collision/navigation cache was empty'
    assert len(re.findall(r'Developer entity \d+ count = 7', text)) == 2
    assert list(map(float, re.search(r'Developer entity \d+ origin = ([^\n]+)', text)[1].split())) == [4, 5, 129]
    saved = [(base / 'maps' / f'{map_name}.dev.{i:03d}.ent').read_bytes() for i in range(4)]
    assert tokens(saved[0]) == tokens(original), 'original/unknown map keys changed'
    assert tokens(saved[2]) == tokens(original), 'deleting the new entity changed original records'
    assert tokens(saved[1]) == tokens(saved[3]), 'saved entity fields did not survive reload'
    assert b'"targetname" "dev_tools_entity_test"' in saved[1]
    for i in range(4):
        shutil.copyfile(base / 'maps' / f'{map_name}.dev.{i:03d}.ent', args.output / f'entities-{i}.ent')
print('PASS: spawn/edit/delete/save/reload, numbered saves and preservation of every original map key')
