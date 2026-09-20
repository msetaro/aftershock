#!/usr/bin/env python3
"""Check the declarative level contract and its deterministic MAP output."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import struct
import sys
import tempfile

from run import ROOT, compare

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compile', action='store_true', help='also compare repeated pinned BSP/AAS builds and committed output')
parser.add_argument('--record-fixtures', action='store_true', help='explicitly author only the new level compiler fixtures')
args = parser.parse_args()
if args.record_fixtures and (not args.compile or os.environ.get('CI')):
    parser.error('fixture recording requires --compile outside CI')

fixture = ROOT / 'tests/assets/levels'
source = json.loads((fixture / 'two_lane.json').read_text())
for name, expected in json.loads((fixture / 'provenance.json').read_text())['files'].items():
    assert hashlib.sha256((fixture / 'assets' / name).read_bytes()).hexdigest() == expected, name


with tempfile.TemporaryDirectory(prefix='aftershock-level-') as temporary:
    folder = Path(temporary)
    shutil.copytree(fixture / 'assets', folder / 'assets')
    path = folder / 'level.json'

    def compile_level(document, output, failure=None, full=False):
        path.write_text(json.dumps(document))
        result = subprocess.run([sys.executable, str(ROOT / 'tools/level'), str(path),
                                 '--output', str(output), *([] if full else ['--map-only'])], cwd=ROOT,
                                text=True, capture_output=True)
        if failure:
            assert result.returncode and failure in result.stderr.lower(), (failure, result.stdout, result.stderr)
            assert not result.stdout.strip(), 'failed compile emitted a success report'
            return None
        assert result.returncode == 0, result.stderr
        report = json.loads(result.stdout)
        assert report['version'] == 1 and report['name'] == 'two_lane'
        if not full:
            assert report['bsp'] is None and report['aas'] is None
        assert report['report']['rooms'] == 3 and report['report']['connections'] == 4
        assert report['report']['reachable_spawns'] == 4
        generated = output / report['map']
        assert generated.is_file()
        assert hashlib.sha256(generated.read_bytes()).hexdigest() == report['sha256']['map']
        result = {}
        for kind in ('map', 'bsp', 'aas') if full else ('map',):
            result[kind] = (output / report[kind]).read_bytes()
            assert hashlib.sha256(result[kind]).hexdigest() == report['sha256'][kind]
        return result

    a = compile_level(source, folder / 'a')
    b = compile_level(source, folder / 'b')
    assert a == b, 'MAP output depends on the output directory or run'
    text = a['map'].decode()
    for classname in ('worldspawn', 'info_player_deathmatch', 'team_CTF_redplayer',
                      'team_CTF_blueplayer', 'func_door', 'misc_model', 'weapon_shotgun', 'light'):
        assert f'"classname" "{classname}"' in text, classname
    assert str(folder) not in text, 'MAP embeds a machine-specific path'

    changed = copy.deepcopy(source)
    changed['connections'][0]['width'] = 16
    compile_level(changed, folder / 'invalid-width', 'corridor width')
    changed = copy.deepcopy(source)
    changed['connections'][0]['height'] = 32
    compile_level(changed, folder / 'invalid-door', 'door height')
    changed = copy.deepcopy(source)
    changed['spawns'][0]['origin'] = [10000, 10000, 24]
    compile_level(changed, folder / 'outside', 'outside')
    changed = copy.deepcopy(source)
    changed['connections'] = changed['connections'][:2]
    compile_level(changed, folder / 'unreachable', 'unreachable spawn')
    changed = copy.deepcopy(source)
    changed['materials']['wall'] = 'level/absent'
    compile_level(changed, folder / 'missing', 'missing material')
    changed = copy.deepcopy(source)
    changed['rules']['max_sightline'] = 128
    compile_level(changed, folder / 'sightline', 'sightline')
    changed = copy.deepcopy(source)
    changed['rules']['max_cover_gap'] = 16
    compile_level(changed, folder / 'cover', 'cover gap')
    changed = copy.deepcopy(source)
    changed['rooms'][1]['id'] = 'west'
    compile_level(changed, folder / 'duplicate', 'duplicate')
    changed = copy.deepcopy(source)
    changed['cover'].append({'id':'barrier','origin':[640,0,0],'kit':'tall','size':[32,512,192]})
    compile_level(changed, folder / 'blocked', 'unreachable spawn')
    changed = copy.deepcopy(source)
    changed['props'][0]['size'] = [16,16,16]
    compile_level(changed, folder / 'prop-size', 'geometry outside')
    changed = copy.deepcopy(source)
    changed['lighting']['lights'][0]['origin'] = [10000,0,24]
    compile_level(changed, folder / 'outside-light', 'outside')
    changed = copy.deepcopy(source)
    changed['pickups'][0]['origin'] = [10000,0,24]
    compile_level(changed, folder / 'outside-pickup', 'outside')
    changed = copy.deepcopy(source)
    changed['unknown'] = True
    compile_level(changed, folder / 'unknown-field', 'unknown fields')
    changed = copy.deepcopy(source)
    changed['spawns'][0]['origin'][0] = float('nan')
    compile_level(changed, folder / 'nonfinite', 'expected integer')
    changed = copy.deepcopy(source)
    changed['materials']['wall'] = '../escape'
    compile_level(changed, folder / 'asset-path', 'relative asset path')
    changed = copy.deepcopy(source)
    changed['rooms'][1]['origin'][0] = 512
    compile_level(changed, folder / 'touching-rooms', '32 units')
    # Rotate the entire layout to exercise the equally supported y-axis generator.
    rotated = copy.deepcopy(source)
    for key in ('rooms','cover','props','spawns','pickups'):
        for item in rotated[key]:
            item['origin'][0],item['origin'][1] = item['origin'][1],item['origin'][0]
            if 'size' in item:
                item['size'][0],item['size'][1] = item['size'][1],item['size'][0]
    for c in rotated['connections']:
        c['axis'] = 'y'
    for light in rotated['lighting']['lights']:
        light['origin'][0],light['origin'][1] = light['origin'][1],light['origin'][0]
    compile_level(rotated, folder / 'rotated')
    if args.compile:
        a = compile_level(source, folder / 'full-a', full=True)
        b = compile_level(source, folder / 'full-b', full=True)
        assert a == b, 'BSP/AAS output depends on build directory or run'
        assert struct.unpack_from('<4sI', a['bsp']) == (b'IBSP', 46)
        assert struct.unpack_from('<4sI', a['aas']) == (b'EAAS', 5)
        if args.record_fixtures:
            (ROOT / 'tests/golden/levels').mkdir(parents=True, exist_ok=True)
        for kind, data in a.items():
            compare('levels/two_lane.' + kind, data, args.record_fixtures)
        print('PASS: repeated MAP/BSP/AAS bytes match the reviewed level fixtures')
print('PASS: owned two-lane level, deterministic MAP, clearances, connectivity, assets and design-rule controls')
