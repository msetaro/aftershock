#!/usr/bin/env python3
"""Fire a cooked weapon and hear authored near/far layers and map reverb on another client."""
import argparse
import json
import math
import os
from pathlib import Path
import re
import pty
import signal
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import time
import wave
from cook import cook
from run import ROOT, SCRATCH, content_settings

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True, help='AFTERSHOCK_DEVTOOLS client')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--output', type=Path, default=SCRATCH / 'aftershock-audio-weapons')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
os.environ['SDL_AUDIODRIVER'] = 'dummy'
with tempfile.TemporaryDirectory(prefix='aftershock-audio-weapons-') as temporary:
    root = Path(temporary)
    owned, source = root/'owned', root/'source'
    source.mkdir()
    fixture = ROOT/'tests/assets/levels'
    level = json.loads((fixture/'two_lane.json').read_text())
    level['name'] = 'audio_weapon_room'
    level['audio_zones'] = [dict(mins=[-256,-256,0], maxs=[256,256,192], wet=.6, decay=.5, damping=.3)]
    shutil.copytree(fixture/'assets', source/'assets')
    level_path = source/'level.json'
    level_path.write_text(json.dumps(level))
    with (args.output/'compile.json').open('w') as log:
        subprocess.run([sys.executable, str(ROOT/'tools/level'), str(level_path), '--output', str(owned)],
                       cwd=ROOT, stdout=log, check=True, timeout=600)
    cook(ROOT/'tests/assets/range.json', owned)
    assets = []
    for role, frequency in [('mechanical',220), ('tail',440), ('distant',880)]:
        with wave.open(str(source/f'{role}.wav'), 'wb') as stream:
            stream.setparams((1,2,48000,9600,'NONE','not compressed'))
            stream.writeframes(b''.join(struct.pack('<h',round(8000*math.sin(i*math.tau*frequency/48000)))
                                       for i in range(9600)))
        assets.append(dict(name=f'sound/{role}', kind='audio', source=f'{role}.wav'))
    event = dict(version=1, name='layered_rifle', bus='weapons', group=0, priority=100,
                 voice_limit=8, distance_model='inverse', reference_distance=80, max_distance=4096,
                 rolloff=1, doppler=False, occlusion=True, reverb_send=.5, layers=[
                     dict(role='mechanical',sample='sound/mechanical.wav',gain=1,min_distance=0,max_distance=256),
                     dict(role='tail',sample='sound/tail.wav',gain=1,min_distance=0,max_distance=1024),
                     dict(role='distant',sample='sound/distant.wav',gain=1,min_distance=256,max_distance=4096)])
    (source/'event.json').write_text(json.dumps(event))
    weapon = json.loads((ROOT/'tests/assets/weapons/rifle.weapon.json').read_text())
    weapon.update(name='audio_rifle',fire_mode='semi',damage=0,minimum_damage=0,interval_ms=500)
    weapon['sounds']['shot'] = 'sound/layered.asevt'
    (source/'weapon.json').write_text(json.dumps(weapon))
    assets += [dict(name='sound/layered',kind='sound-event',source='event.json'),
               dict(name='weapons/audio',kind='weapon',source='weapon.json')]
    project = source/'assets.json'
    project.write_text(json.dumps(dict(version=1,assets=assets)))
    cook(project, owned)
    # Only owned output is copied; installed game paks remain external symlinks.
    assert not list(owned.glob('*.pk3'))
    game = 'baseoa' if args.content == 'openarena' else 'baseq3'
    paks = sorted(args.data.resolve().glob('*.pk3'))
    icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
    assert paks and len(icds) == 1
    env = dict(os.environ, TERM='xterm', LC_ALL='C', LP_NUM_THREADS='1',
               VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as reservation:
        reservation.bind(('127.0.0.1',0))
        port = reservation.getsockname()[1]
    clients, terminals, logs = {}, {}, []
    def log_text(role):
        return (args.output/f'{role}.log').read_text(errors='replace').replace(']\b \b', '').replace('\b \b', '')
    def wait_for(role, token, seconds=30):
        deadline = time.monotonic() + seconds
        while token not in log_text(role):
            assert clients[role].poll() is None and time.monotonic() < deadline, (role,token,log_text(role)[-2000:])
            time.sleep(.1)
    def command(role, text):
        os.write(terminals[role], (text+'\n').encode())
    try:
        for role in ['shooter','listener']:
            home = root/role
            shutil.copytree(owned,home/game)
            for pak in paks:
                (home/game/pak.name).symlink_to(pak)
            master,slave = pty.openpty()
            terminals[role] = master
            logs.append((args.output/f'{role}.log').open('w'))
            start = ['+set','net_port',str(port),'+set','sv_maxclients','2',
                     '+set','g_weapons','weapons/audio.asweapon','+devmap','audio_weapon_room'] if role == 'shooter' else [
                         '+set','net_port','0','+connect',f'127.0.0.1:{port}']
            try:
                clients[role] = subprocess.Popen(['xvfb-run','-a',str(args.binary.resolve()),
                    '+set','fs_basepath',str(home),'+set','fs_homepath',str(home),*content_settings(args.content),
                    '+set','sv_pure','0','+set','net_enabled','1','+set','net_ip','127.0.0.1',
                    '+set','r_mode','3','+set','r_fullscreen','0','+set','s_initsound','1',
                    '+set','s_khz','48','+set','cg_weaponTrace','1','+set','cl_autoRecordDemo','0',
                    '+set','com_maxfps','20','+set','com_maxfpsUnfocused','20',*start],
                    cwd=ROOT,env=env,stdin=slave,stdout=logs[-1],stderr=subprocess.STDOUT,start_new_session=True)
            finally:
                os.close(slave)
            wait_for(role,'Started tty console')
            wait_for(role,'CL_InitCGame:')
        wait_for('listener','Weapon client state: owner=1')
        command('shooter','god; noclip; setviewpos -160 -160 56 0')
        command('listener','god; noclip')
        time.sleep(2)
        rows = []
        for label,position in [('near','-80 -160 56 180'),('far','1696 160 56 180')]:
            command('listener',f'setviewpos {position}; wait 60; s_stop; echo audio_weapon_{label}_ready')
            wait_for('listener',f'\naudio_weapon_{label}_ready\n')
            command('shooter','+attack; wait 2; -attack')
            time.sleep(2)
            command('listener',f's_audioInfo; echo audio_weapon_{label}_done')
            wait_for('listener',f'\naudio_weapon_{label}_done\n')
            text = log_text('listener').split(f'\naudio_weapon_{label}_ready\n')[-1]
            sounds = re.findall(r'Weapon sound: owner=0 .*name=shot ',text)
            assert len(sounds) == 1, (label,sounds,text[-3000:])
            match = re.search(r'Audio events:.*active=(\d+).*started=(\d+).*peak=([\d.]+).*zone=(-?\d+).*wetPeak=([\d.]+).*layers=(\d+),(\d+),(\d+)',text)
            assert match, text[-3000:]
            rows.append(match.groups())
        near,far = rows
        assert near[:2] == far[:2] == ('0','1'), rows
        assert float(near[2]) > 1000 and float(far[2]) > 1, rows
        assert near[3] == '0' and float(near[4]) > 1 and far[3] == '-1' and float(far[4]) == 0, rows
        assert int(near[5]) > 0 and int(near[6]) > 0 and near[7] == '0', near
        assert far[5:7] == ('0','0') and int(far[7]) > 0, far
        print('PASS: remote weapon notifies play mechanical/tail nearby, distant far away, wet room and dry outdoors',rows)
    finally:
        for role,client in clients.items():
            if client.poll() is None:
                command(role,'quit')
                try:
                    client.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(client.pid,signal.SIGKILL)
                    client.wait(timeout=5)
        for terminal in terminals.values():
            os.close(terminal)
        for log in logs:
            log.close()
