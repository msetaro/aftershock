#!/usr/bin/env python3
"""Cook an owned, data-authored animation state machine and its dependencies."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
from run import SCRATCH
import shlex
import struct
import tempfile

from cook import cook, source_assets
from run import run

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=(SCRATCH / 'aftershock-animation'))
parser.add_argument('--cc', default='gcc')
parser.add_argument('--cxx', default='g++')
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
os.environ['CXX'] = args.cxx
sha_object = args.output / 'sha256.o'
run([*shlex.split(args.cc), '-std=c99', '-O2', '-c',
     'third_party/sha256/sha-256.c', '-o', sha_object])
probe = args.output / 'native-probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-ffp-contract=off', '-fno-fast-math', '-Wall', '-Wextra', '-Werror',
     '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/animation.cpp', 'engine/animation/animation.cpp', 'engine/render/tr_cooked.cpp', sha_object,
     '-o', probe])
render_probe = args.output / 'render-probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-DUSE_VULKAN_API', '-ffunction-sections', '-fdata-sections',
     '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/animation_render.cpp', 'engine/qcommon/q_shared.cpp',
     'engine/qcommon/q_math.cpp', 'engine/render/tr_cooked.cpp', sha_object,
     '-Wl,--gc-sections', '-o', render_probe])
run([render_probe])
snapshot_probe = args.output / 'snapshot-probe'
run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
     '-ffunction-sections', '-fdata-sections', '-fno-strict-aliasing',
     '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
     'tests/probes/animation_snapshot.cpp', 'engine/qcommon/msg.cpp',
     'engine/qcommon/huffman.cpp', 'engine/qcommon/huffman_static.cpp',
     'engine/qcommon/q_shared.cpp', '-Wl,--gc-sections', '-o', snapshot_probe])
run([snapshot_probe])
fixture = Path(__file__).resolve().parent / 'assets/animation'
provenance = json.loads((fixture / 'provenance.json').read_text())
for name, expected in provenance['files'].items():
    assert hashlib.sha256((fixture / name).read_bytes()).hexdigest() == expected
owned_output = args.output / 'owned'
cook(fixture / 'rigs.json', owned_output)
run([probe, owned_output / 'cook.index', 'index'])
for name, joints, clips in [('rifle', 13, 6), ('body', 16, 10)]:
    data = (owned_output / ('models/anim_' + name + '.iqm')).read_bytes()
    header = struct.unpack_from('<27I', data, 16)
    assert header[13] == joints and header[17] == clips and header[19] == clips * 31
    run([probe, owned_output / ('animations/anim_' + name + '.asanim'), name])
with tempfile.TemporaryDirectory(prefix='aftershock-animation-source-') as temporary:
    source = Path(temporary)
    project, _ = source_assets(source)
    definition = {
        'version': 1, 'model_source': 'rig.gltf', 'model_asset': 'models/rig.iqm',
        'parameters': [{'name': 'active', 'default': 0}],
        'initial_state': 'idle',
        'states': [
            {'name': 'idle', 'clip': 'idle', 'loop': True,
             'events': [{'time_ms': 250, 'name': 'step', 'bone': 'root'}]},
            {'name': 'wave', 'clip': 'wave', 'loop': False,
             'events': [{'time_ms': 0, 'name': 'start', 'bone': 'tip'},
                        {'time_ms': 500, 'name': 'marker', 'bone': 'tip'},
                        {'time_ms': 1000, 'name': 'finish', 'bone': 'tip'}]}],
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
    assert before[112:144] == hashlib.sha256((args.output / 'models/rig.iqm').read_bytes()).digest()
    manifest = json.loads((args.output / 'animations/rig.manifest.json').read_text())
    assert {'rig.animation.json', 'rig.gltf', 'rig.bin'} <= {item['path'] for item in manifest['inputs']}
    assert cook(project, args.output)['built'] == []
    run([probe, binary])
    definition['transitions'][0]['blend_ms'] = 250
    graph.write_text(json.dumps(definition))
    result = cook(project, args.output)
    assert result['built'] == ['animations/rig'] and result['skipped'] == ['models/rig']
    assert binary.read_bytes() != before
    # A new ordinary source clip combines translation and a quarter turn.
    gltf_path = source / 'rig.gltf'
    gltf = json.loads(gltf_path.read_text())
    turn = copy.deepcopy(gltf['animations'][1])
    turn['name'] = 'turn'
    turn['channels'][0]['target']['node'] = 0
    turn['samplers'].append(copy.deepcopy(gltf['animations'][0]['samplers'][0]))
    turn['channels'].append({'sampler': 1, 'target': {'node': 0, 'path': 'translation'}})
    gltf['animations'].append(turn)
    gltf_path.write_text(json.dumps(gltf))
    definition['states'][0]['events'].insert(0, {'time_ms': 0, 'name': 'entry', 'bone': 'root'})
    graph.write_text(json.dumps(definition))
    cook(project, args.output)
    run([probe, binary, 'edges'])
    definition['parameters'].append({'name': 'upper', 'default': 0, 'min': 0, 'max': 1})
    definition['masks'] = [{'name': 'upper', 'root': 'tip'}]
    definition['nodes'] = [
        {'name': 'idle', 'clip': 'idle', 'loop': True},
        {'name': 'wave', 'clip': 'wave'},
        {'name': 'lower', 'blend': ['idle', 'wave'], 'parameter': 'active'},
        {'name': 'upper', 'additive': ['lower', 'wave'], 'reference': 'idle',
         'parameter': 'upper', 'mask': 'upper'}]
    definition['states'][0]['node'] = 'upper'
    definition['transitions'] = []
    graph.write_text(json.dumps(definition))
    cook(project, args.output)
    run([probe, binary, 'trees'])
print('PASS: versioned animation cooking and incremental source dependencies')
