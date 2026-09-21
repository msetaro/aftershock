#!/usr/bin/env python3
"""Play a cooked event through the real mixer and SDL dummy output device."""
import argparse
import json
import math
import os
from pathlib import Path
import re
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
    for pak in paks:
        (base / pak.name).symlink_to(pak)
    source = home / 'source'
    source.mkdir()
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
                'disconnect', 'quit']
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
assert len(stats) == 3, 'all authored playback checkpoints must run'
assert all(row[:4] == ('1', '1', '28800', '0') for row in stats), stats
assert [int(row[4]) for row in stats] == [1, 2, 3], stats
assert all(int(row[5]) >= (i + 1) * 14400 and float(row[6]) > 1000 for i, row in enumerate(stats)), stats
assert 'Audio event rejected' not in text
print('PASS: cooked event PCM reaches output, playback cache stays fixed, HRTF voice retires')
