#!/usr/bin/env python3
"""Exercise cooked weapons and replicated state in the actual native game."""
import argparse
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
    cook(ROOT / 'tests/assets/weapons/assets.json', base)
    (base / 'weapons.cfg').write_text('\n'.join([
        'set g_weapons weapons/range_rifle.asweapon',
        'set g_weaponTrace 1', 'set cg_weaponTrace 1',
        'set fixedtime 20', 'set sv_fps 50',
        f'devmap {content_maps(args.content)[0]}', 'wait 60',
        '+attack', 'wait 30', '-attack', '+button12', 'wait 15',
        '+button13', 'wait 2', '-button13', 'wait 65', '-button12',
        '+button14', 'wait 30', '-button14', 'wait 10', 'quit']) + '\n')
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
    predictions = re.findall(r'Weapon prediction: hand=0 tick=\d+ equal=(\d)', text)
    assert len(predictions) >= 50 and set(predictions) == {'1'}, (len(predictions), predictions)
    assert not any(error in text for error in ('ERROR:', 'Signal caught', 'Weapon rejected'))
print('PASS: cooked weapon selection, firing/reload/ADS/melee and authoritative client state')
