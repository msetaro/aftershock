#!/usr/bin/env python3
"""Exercise cooked weapons and replicated state in the actual native game."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

from cook import cook
from run import ROOT, content_maps, content_settings

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-weapons-runtime'))
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if not paks or len(icds) != 1:
    parser.error('installed content and one lavapipe ICD are required')
with tempfile.TemporaryDirectory(prefix='aftershock-weapons-live-') as temporary:
    home = Path(temporary)
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    sources = home / 'sources'
    sources.mkdir()
    fixture = ROOT / 'tests/assets/weapons'
    definition = json.loads((fixture / 'rifle.weapon.json').read_text())
    (sources / 'rifle.weapon.json').write_text(json.dumps(definition))
    (sources / 'second.weapon.json').write_text(json.dumps(dict(definition, name='range_rifle_second', damage=55)))
    grenade = dict(definition, name='range_grenade', ballistics='projectile', fire_mode='semi', damage=20,
                   magazine=4, reserve=8, projectile=dict(definition['projectile'], speed=320, fuse_ms=1000))
    (sources / 'grenade.weapon.json').write_text(json.dumps(grenade))
    project = json.loads((fixture / 'assets.json').read_text())
    project['assets'].append({'name': 'weapons/second', 'kind': 'weapon', 'source': 'second.weapon.json'})
    project['assets'].append({'name': 'weapons/grenade', 'kind': 'weapon', 'source': 'grenade.weapon.json'})
    (sources / 'assets.json').write_text(json.dumps(project))
    cook(sources / 'assets.json', base)
    cook(ROOT / 'tests/assets/range.json', base)
    (base / 'weapons.cfg').write_text('\n'.join([
        'set g_weapons "weapons/range_rifle.asweapon weapons/second.asweapon weapons/grenade.asweapon"',
        'set g_weaponTrace 1', 'set cg_weaponTrace 1',
        'set fixedtime 20', 'set sv_fps 50', 'set g_rewind 1', 'set g_rewindTrace 1',
        f'devmap {content_maps(args.content)[0]}', 'wait 60',
        'cmd weapon_attachment 0 1', 'wait 15',
        'rewind_target 0', 'wait 2', '+attack', 'wait 130', '-attack', '+button12', 'wait 15',
        '+button13', 'wait 2', '-button13', 'wait 65', '-button12',
        '+button14', 'wait 30', '-button14', 'wait 10', 'weapon 2', 'wait 15',
        '+attack', 'wait 14', '-attack', 'weapon 1', 'wait 15',
        'weapon 3', 'wait 15', '+attack', 'wait 2', '-attack', 'wait 70', 'quit']) + '\n')
    log = args.output / 'client.log'
    env = dict(os.environ, LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
    with log.open('wb') as stream:
        subprocess.run(['timeout', '60', 'xvfb-run', '-a', str(args.binary.resolve()),
            '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home),
            *content_settings(args.content), '+set', 'net_enabled', '0', '+set', 'sv_pure', '0',
            '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0', '+set', 's_initsound', '0',
            '+set', 'com_maxfps', '0', '+set', 'cl_autoRecordDemo', '0', '+exec', 'weapons.cfg'],
            cwd=ROOT, env=env, stdout=stream, stderr=subprocess.STDOUT, check=True)
    text = log.read_text()
    assert 'Weapon server definition: index=0 name=range_rifle' in text, log
    assert 'Weapon client definition: index=0 name=range_rifle' in text, log
    pattern = r'Weapon %s state: owner=0 hand=0 tick=(\d+) sequence=(\d+) magazine=(\d+) reserve=(\d+) chamber=(\d+) ads=(\d+)'
    server = {row[0]: row[1:] for row in re.findall(pattern % 'server', text)}
    client = {row[0]: row[1:] for row in re.findall(pattern % 'client', text)}
    assert len(client) >= 50 and client.keys() <= server.keys(), (len(client), len(server))
    assert all(server[tick] == state for tick, state in client.items()), 'authoritative weapon state differs'
    assert any(int(row[0]) >= 5 for row in server.values()), 'did not fire the data weapon'
    assert any(row[-1] == '65536' for row in server.values()), 'ADS did not complete'
    assert all(f'Weapon event: owner=0 hand=0 kind={kind}' in text for kind in (0, 2, 3)), 'shot/reload/melee missing'
    hits = re.findall(r'Weapon damage: owner=0 target=\d+ tick=\d+ damage=(\d+)', text)
    assert hits and all(int(value) in (40, 55) for value in hits), hits
    assert 'Rewind trace: shooter=0 ' in text, 'data hitscan did not use rewind'
    switches = re.findall(r'Weapon switch: owner=0 hand=0 from=(\d+) to=(\d+) magazine=(\d+)', text)
    assert switches == [('0', '1', '30'), ('1', '0', '29'), ('0', '2', '4')], switches
    assert 'Weapon client definition: index=1 name=range_rifle_second' in text
    assert 'Weapon impact: material=effects/range_default' in text, 'material impact presentation missing'
    assert 'Weapon attachment: owner=0 hand=0 mask=1 spread=0.750000 fov=45.000000' in text
    assert 'Weapon attachment client: owner=0 hand=0 definition=0 mask=1' in text
    assert 'Weapon projectile server: owner=0 hand=0 sequence=1 ' in text, 'projectile actor missing'
    assert 'Weapon projectile client: owner=0 hand=0 sequence=1 ' in text, 'projectile snapshot rendering missing'
    assert 'Weapon projectile exploded: owner=0 sequence=1 ' in text, 'projectile never detonated'
    assert text.count('Weapon projectile predicted: hand=0 sequence=1 ') == 1, 'missing or duplicated local projectile'
    corrections = re.findall(r'Weapon projectile correction: hand=0 sequence=1 error=([0-9.]+)', text)
    assert corrections and max(map(float, corrections)) <= 1, corrections
    predictions = re.findall(r'Weapon prediction: hand=0 tick=\d+ equal=(\d)', text)
    assert len(predictions) >= 50 and set(predictions) == {'1'}, (len(predictions), predictions)
    animations = re.findall(r'Weapon animation prediction: hand=0 tick=\d+ equal=(\d)', text)
    assert len(animations) >= 50 and set(animations) == {'1'}, (len(animations), animations)
    assert 'Weapon animation server: owner=0 hand=0 state=reload' in text
    assert 'Weapon animation client: owner=0 hand=0 state=reload' in text
    assert not any(error in text for error in ('ERROR:', 'Signal caught', 'Weapon rejected'))
print('PASS: cooked weapon selection, firing/reload/ADS/melee and authoritative client state')
