#!/usr/bin/env python3
"""Cook an owned, data-authored animation state machine and its dependencies."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile

from cook import cook, source_assets

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-animation'))
args = parser.parse_args()
args.output = args.output.resolve()
fixture = Path(__file__).resolve().parent / 'assets/animation'
provenance = json.loads((fixture / 'provenance.json').read_text())
for name, expected in provenance['files'].items():
    assert hashlib.sha256((fixture / name).read_bytes()).hexdigest() == expected
owned_output = args.output / 'owned'
cook(fixture / 'assets.json', owned_output)
for name, joints, clips in [('rifle', 13, 6), ('body', 16, 10)]:
    data = (owned_output / ('models/anim_' + name + '.iqm')).read_bytes()
    header = struct.unpack_from('<27I', data, 16)
    assert header[13] == joints and header[17] == clips and header[19] == clips * 31
with tempfile.TemporaryDirectory(prefix='aftershock-animation-source-') as temporary:
    source = Path(temporary)
    project, _ = source_assets(source)
    definition = {
        'version': 1, 'model_source': 'rig.gltf', 'model_asset': 'models/rig.iqm',
        'parameters': [{'name': 'active', 'default': 0}],
        'initial_state': 'idle',
        'states': [
            {'name': 'idle', 'clip': 'idle', 'loop': True},
            {'name': 'wave', 'clip': 'wave', 'loop': False,
             'events': [{'time_ms': 500, 'name': 'marker', 'bone': 'tip'}]}],
        'transitions': [
            {'from': 'idle', 'to': 'wave', 'blend_ms': 150,
             'conditions': [{'parameter': 'active', 'op': '>', 'value': 0.5}]},
            {'from': 'wave', 'to': 'idle', 'blend_ms': 150, 'on_end': True}],
    }
    graph = source / 'rig.animation.json'
    graph.write_text(json.dumps(definition))
    project.write_text(json.dumps({'version': 1, 'assets': [
        {'name': 'models/rig', 'kind': 'model', 'source': 'rig.gltf', 'scale': 1, 'fps': 2},
        {'name': 'animations/rig', 'kind': 'animation', 'source': graph.name, 'scale': 1, 'fps': 2}]}))
    result = cook(project, args.output)
    assert sorted(result['built'] + result['skipped']) == ['animations/rig', 'models/rig']
    binary = args.output / 'animations/rig.asanim'
    before = binary.read_bytes()
    magic, version, size, hashed = struct.unpack_from('<8sII32s', before)
    assert magic == b'ASANIM\0\0' and version == 1 and size == len(before) - 48
    assert hashed == hashlib.sha256(before[48:]).digest()
    manifest = json.loads((args.output / 'animations/rig.manifest.json').read_text())
    assert {'rig.animation.json', 'rig.gltf', 'rig.bin'} <= {item['path'] for item in manifest['inputs']}
    assert cook(project, args.output)['built'] == []
    definition['transitions'][0]['blend_ms'] = 250
    graph.write_text(json.dumps(definition))
    result = cook(project, args.output)
    assert result['built'] == ['animations/rig'] and result['skipped'] == ['models/rig']
    assert binary.read_bytes() != before
print('PASS: versioned animation cooking and incremental source dependencies')
