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
parser.add_argument('--lifecycle', action='store_true', help='also exercise spectator/respawn record reuse')
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
    script = [
        'set g_weapons "weapons/range_rifle.asweapon weapons/second.asweapon weapons/grenade.asweapon"',
        'set g_weaponTrace 1', 'set cg_weaponTrace 1',
        'set fixedtime 20', 'set sv_fps 50', 'set g_rewind 1', 'set g_rewindTrace 1',
        f'devmap {content_maps(args.content)[0]}', 'wait 60',
        'cmd weapon_attachment 0 1', 'wait 15',
        'rewind_target 0', 'wait 2', '+attack', 'wait 130', '-attack', '+button12', 'wait 15',
        'weapon_status', 'screenshot weapon-ads', 'wait 2', '+button13', 'wait 2', '-button13', 'wait 65', '-button12',
        '+button14', 'wait 30', '-button14', 'wait 10', 'weapon 2', 'wait 15',
        '+attack', 'wait 14', '-attack', 'weapon 1', 'wait 15',
        'weapon 3', 'wait 15', '+attack', 'wait 2', '-attack', 'wait 70', 'weapon_status', 's_list']
    if args.lifecycle:
        script += ['echo weapon_lifecycle_begin', 'team spectator', 'wait 15',
                   'echo weapon_lifecycle_spectator', 'wait 15', 'echo weapon_lifecycle_rejoin',
                   'team free', 'wait 70', '+attack', 'wait 6', '-attack', 'wait 20',
                   'echo weapon_lifecycle_end']
    (base / 'weapons.cfg').write_text('\n'.join([*script, 'quit']) + '\n')
    log = args.output / 'client.log'
    env = dict(os.environ, SDL_AUDIODRIVER='dummy', LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
    with log.open('wb') as stream:
        subprocess.run(['timeout', '60', 'xvfb-run', '-a', str(args.binary.resolve()),
            '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home),
            *content_settings(args.content), '+set', 'net_enabled', '0', '+set', 'sv_pure', '0',
            '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0', '+set', 's_initsound', '1',
            '+set', 'com_maxfps', '0', '+set', 'cl_autoRecordDemo', '0', '+exec', 'weapons.cfg'],
            cwd=ROOT, env=env, stdout=stream, stderr=subprocess.STDOUT, check=True)
    complete = log.read_text()
    text = complete.split('weapon_lifecycle_begin')[0]
    if args.lifecycle:
        actors = re.findall(r'Weapon actor: owner=0 spawn=(\d+) state=(\d+),(\d+) animation=(\d+),(\d+)', complete)
        assert len(actors) >= 2 and len({row[0] for row in actors}) >= 2, actors
        assert len({row[1:] for row in actors}) == 1, 'respawn allocated another auxiliary record set'
        spectator = complete.split('weapon_lifecycle_spectator')[1].split('weapon_lifecycle_rejoin')[0]
        assert 'Weapon server state: owner=0' not in spectator, 'spectator retained an active weapon actor'
        rejoined = complete.split('weapon_lifecycle_rejoin')[1]
        assert 'Weapon event: owner=0 hand=0 kind=0' in rejoined and 'weapon_lifecycle_end' in rejoined
        assert not any(error in complete for error in ('ERROR:', 'Signal caught', 'Weapon rejected'))
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
    views = re.findall(r'Weapon rendering: draws=(\d+) attachments=(\d+) ads=(\d+) error=([0-9.]+) kick=([0-9.]+) fov=([0-9.]+)', text)
    assert views and any(int(v[0]) >= 100 and int(v[1]) >= 100 and int(v[2]) >= 5 and
                         float(v[3]) < 0.02 and float(v[4]) > 0 and float(v[5]) == 45 for v in views), views
    image = base / 'screenshots/weapon-ads.tga'
    assert image.is_file(), 'ADS capture missing'
    (args.output / 'weapon-ads.tga').write_bytes(image.read_bytes())
    notify_pattern = r'Weapon %s: owner=0 hand=0 definition=(\d+) spawn=(\d+) sequence=(\d+) name=(\w+) time=(\d+)'
    audible = {'shot', 'magazine_out', 'magazine_in', 'bolt'}
    notifies = [row for row in re.findall(notify_pattern % 'notify server', text) if row[3] in audible]
    sounds = re.findall(notify_pattern % 'sound', text)
    assert notifies and set(notifies) == set(sounds) and len(sounds) == len(set(sounds)), (len(notifies), len(sounds))
    for sound in ('shot', 'reload'):
        resident = re.search(r'(\d+)\[16bit\] : sound/range_' + sound + r'\.wav \[resident\]', text)
        assert resident and int(resident[1]) > 1000, 'cooked sound was not decoded: ' + sound
    assert 'SDL_Init( SDL_INIT_AUDIO )' in text and 'SDL audio initialized.' in text, 'audio backend did not initialize'
    assert not any(error in text for error in ('ERROR:', 'Signal caught', 'Weapon rejected'))
print('PASS: cooked weapon selection, firing/reload/ADS/melee and authoritative client state')
