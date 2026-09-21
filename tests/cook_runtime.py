#!/usr/bin/env python3
"""Measure watched character texture/model/material reloads through shared controls."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import struct
import sys
import tempfile
import time

from PIL import Image, ImageChops
from run import ROOT, build, content_maps
from window import wait_for
sys.path.insert(0, str(ROOT))
from tools.agent import Engine

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', type=Path)
parser.add_argument('--modules', action='store_true', help='build and exercise the optional renderer module')
parser.add_argument('--output', type=Path)
parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='aftershock-cook-live-', dir=os.environ.get('AFTERSHOCK_SCRATCH')) as temporary:
    output = args.output.resolve() if args.output else Path(temporary) / 'output'
    output.mkdir(parents=True, exist_ok=True)
    binary = args.binary or build(output / 'build', ['BUILD_SERVER=0', 'AFTERSHOCK_DEVTOOLS=1',
                                  f'USE_RENDERER_DLOPEN={int(args.modules)}']) / 'quake3e.x64'
    home = Path(temporary) / 'home'
    base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
    source = Path(temporary) / 'source'
    shutil.copytree(ROOT / 'tests/assets/cook-character', source)
    with (output / 'watch.log').open('wb') as watch_log:
        watcher = subprocess.Popen([sys.executable, 'tools/cook', str(source / 'assets.json'), '--output', str(base), '--watch'],
                                   cwd=ROOT, stdout=watch_log, stderr=subprocess.STDOUT)
        try:
            wait_for(lambda: (base / 'cook.revision').is_file(), watcher)
            with Engine(binary, args.data, args.content, home=home, arguments=['+set', 'dev_reloadAssets', '1']) as engine:
                engine.request('session', dt=8, seed=123)
                engine.request('map', name=content_maps(args.content)[0])
                engine.step(200)
                engine.request('cvar.set', name='timescale', value='0')
                engine.request('panel', name='Animation')
                engine.request('animation.load', path='models/character.iqm')
                engine.step(3)
                snapshots, images = [], {}

                def registry(kind):
                    return engine.request('assets', kind=kind, filter='models/character')['items']

                def reload_count(kind):
                    return sum(item['reloads'] for item in registry(kind))

                def wait_reload(kind, previous):
                    deadline = time.monotonic() + 10
                    while reload_count(kind) <= previous:
                        assert watcher.poll() is None, (output / 'watch.log').read_text()
                        assert time.monotonic() < deadline, f'{kind}: reload timed out'
                        engine.step(2)
                        time.sleep(0.005)
                    engine.step(2)

                def memory():
                    state = engine.request('profile')['memory']
                    renderer = next(tag for tag in state['tags'] if tag['name'] == 'RENDERER')
                    return renderer['bytes'], renderer['blocks'], state['hunkPermanent']

                def capture(name):
                    result = engine.request('capture', name=name)
                    engine.step(2)
                    state = engine.request('editor.state')
                    x, y, width, height = state['animation']['viewport']
                    path = base / result['path']
                    with Image.open(path) as image:
                        images[name] = image.convert('RGB').crop((x, y, x + width, y + height))
                    shutil.copyfile(path, output / (name + '.png'))
                    snapshots.append(state)
                    return state

                initial_memory = memory()
                assert initial_memory[0] > 0
                assert any(image['format'] == 8 for image in registry('images')), 'expected rhiFormat_t::BC7_SRGB'
                capture('before')
                previous = reload_count('images')
                edited = time.monotonic()
                Image.new('RGBA', (16, 16), (32, 240, 64, 255)).save(source / 'character.png')
                wait_reload('images', previous)
                capture('after')
                latency = time.monotonic() - edited
                assert latency < 1.0, f'texture edit took {latency:.3f}s to reach the sampled frame'
                assert snapshots[-1]['animation']['clip'] == 'idle'
                model_handle = snapshots[-1]['animation']['model']
                engine.request('animation.set', field='clip', value=1)
                engine.request('animation.set', field='frame', value=46)
                engine.step(2)
                wave_state = capture('wave')['animation']
                assert wave_state['clip'] == 'wave' and 31 < wave_state['frame'] < 61, wave_state
                document = json.loads((source / 'character.gltf').read_text())
                wave = next(clip for clip in document['animations'] if clip['name'] == 'wave')
                channel = next(channel for channel in wave['channels'] if channel['target']['path'] == 'rotation'
                               and document['nodes'][channel['target']['node']]['name'] == 'arm.L')
                accessor = document['accessors'][wave['samplers'][channel['sampler']]['output']]
                assert accessor['componentType'] == 5126 and accessor['type'] == 'VEC4'
                view = document['bufferViews'][accessor['bufferView']]
                buffer_path = source / document['buffers'][view['buffer']]['uri']
                data = bytearray(buffer_path.read_bytes())
                offset = view.get('byteOffset', 0) + accessor.get('byteOffset', 0)
                stride = view.get('byteStride', 16)
                first = bytes(data[offset:offset + 16])
                for frame in range(accessor['count']):
                    data[offset + frame * stride:offset + frame * stride + 16] = first
                for bound in ('min', 'max'):
                    if bound in accessor:
                        accessor[bound] = list(struct.unpack('<4f', first))
                wave['name'] = 'salute'
                previous = reload_count('models')
                temporary_buffer = buffer_path.with_suffix('.tmp')
                temporary_buffer.write_bytes(data)
                temporary_buffer.replace(buffer_path)

                def publish_document():
                    pending = source / 'character.tmp'
                    pending.write_text(json.dumps(document))
                    pending.replace(source / 'character.gltf')

                publish_document()
                wait_reload('models', previous)
                pose = capture('model_reload')['animation']
                assert pose['clip'] == 'salute' and pose['frame'] == wave_state['frame'] and pose['model'] == model_handle, pose
                engine.request('animation.set', field='clip', value=0)
                engine.request('animation.set', field='frame', value=15)
                engine.step(2)
                idle = capture('idle')['animation']
                assert idle['clip'] == 'idle' and 0 < idle['frame'] < 30
                material = document['materials'][0]
                material['alphaMode'] = 'BLEND'
                material['pbrMetallicRoughness']['baseColorFactor'] = [1, 1, 1, 0]
                previous = reload_count('materials')
                publish_document()
                wait_reload('materials', previous)
                capture('material_reload')
                engine.step(100)
                baseline_memory = memory()
                allocations = engine.request('editor.state')['allocations']
                engine.step(80)
                assert engine.request('editor.state')['allocations'] == allocations, 'idle preview allocated'
                for edit in range(6):
                    previous = reload_count('materials')
                    material['alphaMode'] = 'OPAQUE' if edit % 2 == 0 else 'BLEND'
                    material['pbrMetallicRoughness']['baseColorFactor'][3] = 1 if edit % 2 == 0 else 0
                    publish_document()
                    wait_reload('materials', previous)
                    pose = engine.request('editor.state')['animation']
                    assert (pose['model'], pose['frame'], pose['clip']) == (idle['model'], idle['frame'], idle['clip'])
                    assert memory() == baseline_memory, (memory(), baseline_memory)
                assert reload_count('materials') >= 7
                engine.request('exec', command='vid_restart')
                engine.step(60)
                engine.request('panel', name='Animation')
                engine.request('animation.load', path='models/character.iqm')
                engine.step(3)
                previous = reload_count('materials')
                material['alphaMode'] = 'OPAQUE'
                material['pbrMetallicRoughness']['baseColorFactor'][3] = 1
                publish_document()
                wait_reload('materials', previous)
                pose = capture('restarted')['animation']
                assert pose['model'] > 0 and (pose['frame'], pose['clip']) == (0, 'idle'), pose
                shutil.copyfile(engine.log_path, output / 'client.log')
                def changed(first, second):
                    pixels = ImageChops.difference(images[first], images[second]).tobytes()
                    return sum(max(pixels[i:i + 3]) > 40 for i in range(0, len(pixels), 3))
                texture_pixels = changed('before', 'after')
                assert texture_pixels > 200, texture_pixels
                assert changed('wave', 'model_reload') > 30
                assert changed('idle', 'material_reload') > 200
                assert sum(g > r * 2 and g > b * 1.5 for r, g, b in struct.iter_unpack('BBB', images['restarted'].tobytes())) > 500
                (output / 'latency.txt').write_text(f'{latency:.6f}s source edit to rendered screenshot; {texture_pixels} changed preview pixels\n')
        finally:
            watcher.terminate()
            try:
                watcher.wait(timeout=10)
            except subprocess.TimeoutExpired:
                watcher.kill()
                watcher.wait(timeout=5)
print('PASS: shared character controls, watched reloads within one second, stable handles/memory and video restart')
