#!/usr/bin/env python3
"""Check authored sound layers through the real cooker and incremental cache."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile

from cook import cook
from run import SCRATCH

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=SCRATCH / 'aftershock-audio-events')
args = parser.parse_args()
args.output = args.output.resolve()
with tempfile.TemporaryDirectory(prefix='aftershock-sound-event-') as temporary:
    root = Path(temporary)
    definition = dict(version=1, name='rifle', bus='weapons', group=2, priority=100,
                      voice_limit=8, distance_model='inverse', reference_distance=80,
                      max_distance=4096, rolloff=1, doppler=True, occlusion=True,
                      reverb_send=.25, layers=[
                          dict(role='mechanical', sample='sound/rifle_mech.wav', gain=.5,
                               min_distance=0, max_distance=256),
                          dict(role='tail', sample='sound/rifle_tail.wav', gain=1,
                               min_distance=0, max_distance=1024),
                          dict(role='distant', sample='sound/rifle_far.wav', gain=.75,
                               min_distance=256, max_distance=4096)])
    source = root / 'event.json'
    source.write_text(json.dumps(definition))
    project = root / 'assets.json'
    project.write_text(json.dumps(dict(version=1, assets=[
        dict(name='sound/rifle', kind='sound-event', source='event.json')])))
    assert cook(project, args.output)['built'] == ['sound/rifle']
    data = (args.output / 'sound/rifle.asevt').read_bytes()
    assert struct.unpack_from('<8sII', data) == (b'ASEVENT\0', 1, 400)
    assert len(data) == 448 and hashlib.sha256(data[48:]).digest() == data[16:48]
    assert struct.unpack_from('<7I4f', data, 80) == (0, 2, 100, 8, 1, 3, 3, 80, 4096, 1, .25)
    layers = [struct.unpack_from('<64sI3f', data, 128 + 80 * i) for i in range(4)]
    assert layers[0] == (b'sound/rifle_mech.wav'.ljust(64, b'\0'), 0, .5, 0, 256)
    assert layers[1][1:] == (1, 1, 0, 1024)
    assert layers[2][1:] == (2, .75, 256, 4096)
    assert layers[3] == (bytes(64), 0, 0, 0, 0)
    assert cook(project, args.output)['built'] == []
    definition['layers'][2]['gain'] = .25
    source.write_text(json.dumps(definition))
    assert cook(project, args.output)['built'] == ['sound/rifle']
    assert (args.output / 'sound/rifle.asevt').read_bytes() != data
print('PASS: authored sound-event bus, limits, spatial settings, layers and incremental cook')
