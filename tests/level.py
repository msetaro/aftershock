#!/usr/bin/env python3
"""Check the declarative level contract and its deterministic MAP output."""
import copy
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from run import ROOT

fixture = ROOT / 'tests/assets/levels'
source = json.loads((fixture / 'two_lane.json').read_text())
for name, expected in json.loads((fixture / 'provenance.json').read_text())['files'].items():
    assert hashlib.sha256((fixture / 'assets' / name).read_bytes()).hexdigest() == expected, name


with tempfile.TemporaryDirectory(prefix='aftershock-level-') as temporary:
    folder = Path(temporary)
    shutil.copytree(fixture / 'assets', folder / 'assets')
    path = folder / 'level.json'

    def compile_level(document, output, failure=None):
        path.write_text(json.dumps(document))
        result = subprocess.run([sys.executable, str(ROOT / 'tools/level'), str(path),
                                 '--output', str(output), '--map-only'], cwd=ROOT,
                                text=True, capture_output=True)
        if failure:
            assert result.returncode and failure in result.stderr.lower(), (failure, result.stdout, result.stderr)
            assert not result.stdout.strip(), 'failed compile emitted a success report'
            return None
        assert result.returncode == 0, result.stderr
        report = json.loads(result.stdout)
        assert report['version'] == 1 and report['name'] == 'two_lane'
        assert report['bsp'] is None and report['aas'] is None
        assert report['report']['rooms'] == 3 and report['report']['connections'] == 4
        assert report['report']['reachable_spawns'] == 4
        generated = output / report['map']
        assert generated.is_file()
        assert hashlib.sha256(generated.read_bytes()).hexdigest() == report['sha256']['map']
        return generated.read_bytes()

    a = compile_level(source, folder / 'a')
    b = compile_level(source, folder / 'b')
    assert a == b, 'MAP output depends on the output directory or run'
    text = a.decode()
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
print('PASS: owned two-lane level, deterministic MAP, clearances, connectivity, assets and design-rule controls')
