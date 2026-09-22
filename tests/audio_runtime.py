#!/usr/bin/env python3
"""Play a cooked event through the real mixer and SDL dummy output device."""
import argparse
import json
import math
import os
from pathlib import Path
import re
import shutil
import sys
import struct
import subprocess
import tempfile
import wave

from cook import cook
from run import ROOT, SCRATCH, content_maps, content_settings

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path, required=True)
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--output', type=Path, default=SCRATCH / 'aftershock-audio-runtime')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
assert paks and args.binary.is_file(), 'installed content and an existing client are required'
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
assert len(icds) == 1
env = dict(os.environ, SDL_AUDIODRIVER='dummy', VK_ICD_FILENAMES=str(icds[0]),
           VK_DRIVER_FILES=str(icds[0]), LIBGL_ALWAYS_SOFTWARE='1', LP_NUM_THREADS='1')
with tempfile.TemporaryDirectory(prefix='aftershock-audio-runtime-') as temporary:
    home = Path(temporary)
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    base.mkdir()
    source = home / 'source'
    source.mkdir()
    fixture = ROOT / 'tests/assets/levels'
    level = json.loads((fixture / 'two_lane.json').read_text())
    level['name'] = 'audio_room'
    level['audio_zones'] = [dict(mins=[-256,-256,0], maxs=[256,256,192], wet=.6, decay=.5, damping=.3)]
    shutil.copytree(fixture / 'assets', source / 'assets')
    level_path = source / 'level.json'
    level_path.write_text(json.dumps(level))
    with (args.output / 'compile.json').open('w') as log:
        subprocess.run([sys.executable, str(ROOT / 'tools/level'), str(level_path), '--output', str(base)],
                       cwd=ROOT, stdout=log, check=True, timeout=600)
    shutil.copyfile(base / 'compile.log', args.output / 'compile.log')
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    with wave.open(str(source / 'tone.wav'), 'wb') as stream:
        stream.setparams((1, 2, 48000, 14400, 'NONE', 'not compressed'))
        stream.writeframes(b''.join(struct.pack('<h', round(8000 * math.sin(i * math.tau * 440 / 48000)))
                                  for i in range(14400)))
    definition = dict(version=1, name='runtime', bus='weapons', group=0, priority=100,
                      voice_limit=4, distance_model='inverse', reference_distance=80,
                      max_distance=4096, rolloff=1, doppler=True, occlusion=True,
                      reverb_send=.25, layers=[dict(role='mechanical', sample='sound/audio_test.wav',
                                                  gain=1, min_distance=0, max_distance=4096)])
    (source / 'event.json').write_text(json.dumps(definition))
    project = source / 'assets.json'
    project.write_text(json.dumps(dict(version=1, assets=[
        dict(name='sound/audio_test', kind='audio', source='tone.wav'),
        dict(name='sound/audio_event', kind='sound-event', source='event.json')])))
    cook(project, base)
    commands = [f'map {content_maps(args.content)[0]}', 'wait 120', 's_stop',
                'play sound/audio_event.asevt', 'wait 90', 's_audioInfo',
                'play sound/audio_event.asevt', 'wait 90', 's_audioInfo',
                'set s_hrtf 1', 'play sound/audio_event.asevt', 'wait 90', 's_audioInfo',
                'devmap audio_room', 'wait 120', 'setviewpos -160 -160 24 0', 'wait 10', 's_stop',
                'play sound/audio_event.asevt', 'wait 90', 's_audioInfo',
                'setviewpos 1696 160 56 180', 'wait 10', 's_stop',
                'play sound/audio_event.asevt', 'wait 90', 's_audioInfo',
                'setviewpos -160 -160 24 0', 'wait 10', 's_stop',
                's_event sound/audio_event.asevt 768 0 50', 'wait 90', 's_audioInfo',
                's_stop', 's_stream 0 music sound/audio_test.wav 1',
                's_stream 1 ambient sound/audio_test.wav 1', 'wait 240', 's_streamInfo', 's_audioInfo',
                'wait 240', 's_streamInfo', 's_audioInfo', 's_streamStop 0', 's_streamStop 1',
                'wait 10', 's_streamInfo', 's_audioInfo', 'disconnect', 'quit']
    (base / 'audio-test.cfg').write_text('\n'.join(commands) + '\n')
    command = ['xvfb-run', '-a', str(args.binary.resolve()), '+set', 'fs_basepath', str(home),
               '+set', 'fs_homepath', str(home), *content_settings(args.content),
               '+set', 'r_fullscreen', '0', '+set', 'r_mode', '3', '+set', 's_initsound', '1',
               '+set', 's_khz', '48', '+set', 'sv_pure', '0', '+set', 'net_ip', '127.0.0.1',
               '+set', 'com_maxfps', '125', '+set', 'com_maxfpsUnfocused', '125',
               '+set', 'cl_autoRecordDemo', '0', '+exec', 'audio-test.cfg']
    with (args.output / 'client.log').open('wb') as log:
        subprocess.run(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT,
                       check=True, timeout=120)
text = (args.output / 'client.log').read_text(errors='replace')
stats = re.findall(r'Audio events: events=(\d+) samples=(\d+) bytes=(\d+) active=(\d+) started=(\d+) mixed=(\d+) peak=([\d.]+)', text)
assert len(stats) == 9, 'all authored playback checkpoints must run'
assert all(row[:4] == ('1', '1', '28800', '0') for row in stats), stats
assert [int(row[4]) for row in stats[:3]] == [1, 2, 3], stats
assert all(int(row[5]) >= (i + 1) * 14400 and float(row[6]) > 1000 for i, row in enumerate(stats[:3])), stats
acoustics = re.findall(r'zones=(\d+) zone=(-?\d+) wet=([\d.]+) traced=(\d+) blocked=(\d+) wetPeak=([\d.]+)', text)
assert len(acoustics) == 9, acoustics
room, outdoors, wall = acoustics[3:6]
assert room[:2] == ('1', '0') and float(room[2]) > .59 and float(room[5]) > 1, room
assert outdoors[:2] == ('1', '-1') and float(outdoors[5]) == 0, outdoors
assert int(wall[3]) > 0 and int(wall[4]) > 0, wall
streams = re.findall(r'Audio streams: prepared=(\d+) active=(\d+) bytes=(\d+) buffers=(\d+) loops=(\d+) reads=(\d+) failures=(\d+)', text)
assert len(streams) == 3, streams
assert all(row[0] == '2' and row[2] == '57600' and row[3] == '40960' and row[6] == '0' for row in streams), streams
assert [row[1] for row in streams] == ['2','2','0'], streams
assert int(streams[1][4]) > int(streams[0][4]) >= 4, streams
assert int(streams[1][5]) > int(streams[0][5]), streams
assert float(stats[7][6]) > 1000 and int(stats[7][5]) > int(stats[6][5]), stats
assert 'Audio event rejected' not in text and 'Audio zone rejected' not in text
print('PASS: cooked PCM playback, fixed cache, HRTF retirement, authored room/outdoor reverb and native wall occlusion')
