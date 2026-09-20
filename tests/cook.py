#!/usr/bin/env python3
"""Cook ordinary static/skinned source assets and check incremental, portable outputs."""
import argparse
import copy
import hashlib
import io
import wave
import json
import os
from pathlib import Path
import struct
import shlex
import subprocess
import sys
import tempfile
import time

from PIL import Image
from run import ROOT


def source_assets(directory):
    """An owned triangle with two joints and two clips; no game content required."""
    blob = bytearray()
    views, accessors = [], []

    def array(values, fmt, component, shape, count):
        blob.extend(b'\0' * (-len(blob) % 4))
        data = struct.pack('<' + fmt * len(values), *values)
        views.append({'buffer': 0, 'byteOffset': len(blob), 'byteLength': len(data)})
        blob.extend(data)
        accessors.append({'bufferView': len(views) - 1, 'componentType': component,
                          'count': count, 'type': shape})
        return len(accessors) - 1

    position = array([0, 0, 0, 1, 2, 0, -1, 2, 0], 'f', 5126, 'VEC3', 3)
    accessors[position].update(min=[-1, 0, 0], max=[1, 2, 0])
    normal = array([0, 0, 1] * 3, 'f', 5126, 'VEC3', 3)
    uv = array([0.5, 0, 1, 1, 0, 1], 'f', 5126, 'VEC2', 3)
    joints = array([0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0], 'B', 5121, 'VEC4', 3)
    weights = array([1, 0, 0, 0] * 3, 'f', 5126, 'VEC4', 3)
    indices = array([0, 1, 2], 'H', 5123, 'SCALAR', 3)
    identity = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
    inverse = identity.copy()
    inverse[13] = -1
    binds = array(identity + inverse, 'f', 5126, 'MAT4', 2)
    times = array([0, 1], 'f', 5126, 'SCALAR', 2)
    accessors[times].update(min=[0], max=[1])
    idle = array([0, 0, 0, 0.125, 0, 0], 'f', 5126, 'VEC3', 2)
    wave = array([0, 0, 0, 1, 0, 0, 0.70710678, 0.70710678], 'f', 5126, 'VEC4', 2)
    document = {
        'asset': {'version': '2.0', 'generator': 'Aftershock owned cooker test'},
        'scene': 0, 'scenes': [{'nodes': [0, 2]}],
        'nodes': [{'name': 'root', 'children': [1]},
                  {'name': 'tip', 'translation': [0, 1, 0]}, {'name': 'triangle', 'mesh': 0, 'skin': 0}],
        'skins': [{'joints': [0, 1], 'skeleton': 0, 'inverseBindMatrices': binds}],
        'meshes': [{'name': 'triangle', 'primitives': [{'attributes': {
            'POSITION': position, 'NORMAL': normal, 'TEXCOORD_0': uv,
            'JOINTS_0': joints, 'WEIGHTS_0': weights}, 'indices': indices}]}],
        'animations': [
            {'name': 'idle', 'samplers': [{'input': times, 'output': idle, 'interpolation': 'LINEAR'}],
             'channels': [{'sampler': 0, 'target': {'node': 0, 'path': 'translation'}}]},
            {'name': 'wave', 'samplers': [{'input': times, 'output': wave, 'interpolation': 'LINEAR'}],
             'channels': [{'sampler': 0, 'target': {'node': 1, 'path': 'rotation'}}]}],
        'buffers': [{'uri': 'rig.bin', 'byteLength': len(blob)}],
        'bufferViews': views, 'accessors': accessors}
    (directory / 'rig.bin').write_bytes(blob)
    (directory / 'rig.gltf').write_text(json.dumps(document))
    packed = copy.deepcopy(document)
    del packed['buffers'][0]['uri']
    encoded = json.dumps(packed, separators=(',', ':')).encode()
    encoded += b' ' * (-len(encoded) % 4)
    binary = bytes(blob) + b'\0' * (-len(blob) % 4)
    chunks = struct.pack('<II', len(encoded), 0x4E4F534A) + encoded
    chunks += struct.pack('<II', len(binary), 0x004E4942) + binary
    (directory / 'rig.glb').write_bytes(struct.pack('<III', 0x46546C67, 2, 12 + len(chunks)) + chunks)
    static = copy.deepcopy(document)
    static.pop('skins')
    static.pop('animations')
    static['nodes'] = [{'mesh': 0}]
    static['scenes'] = [{'nodes': [0]}]
    attributes = static['meshes'][0]['primitives'][0]['attributes']
    attributes.pop('JOINTS_0')
    attributes.pop('WEIGHTS_0')
    (directory / 'static.gltf').write_text(json.dumps(static))
    static['nodes'][0]['scale'] = [-1, 1, 1]
    (directory / 'mirrored.gltf').write_text(json.dumps(static))
    Image.new('RGBA', (16, 16), (128, 96, 255, 255)).save(directory / 'color.png')
    assets = [{'name': 'models/' + name, 'kind': 'model', 'source': source, 'scale': 1, 'fps': 2}
              for name, source in [('rig', 'rig.gltf'), ('packed', 'rig.glb'), ('static', 'static.gltf'), ('mirrored', 'mirrored.gltf')]]
    fixture = ROOT / 'tests/assets/cook-audio'
    provenance = json.loads((fixture / 'provenance.json').read_text())
    for filename, expected in provenance['files'].items():
        assert hashlib.sha256((fixture / filename).read_bytes()).hexdigest() == expected
    for extension in ('wav', 'ogg'):
        (directory / ('tone.' + extension)).write_bytes((fixture / ('tone.' + extension)).read_bytes())
        assets.append({'name': 'sounds/' + extension, 'kind': 'audio', 'source': 'tone.' + extension})
    assets += [{'name': 'textures/' + fmt, 'kind': 'texture', 'source': 'color.png',
                'format': fmt, 'srgb': fmt == 'bc7'} for fmt in ('bc7', 'bc5', 'bc4')]
    project = directory / 'assets.json'
    project.write_text(json.dumps({'version': 1, 'assets': assets}))
    return project, [a['name'] for a in assets]


def cook(project, output):
    command = [sys.executable, 'tools/cook', str(project), '--output', str(output)]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError('cooker failed:\n' + result.stdout + result.stderr)
    return json.loads(result.stdout)


def model_header(data):
    header = struct.unpack_from('<27I', data, 16)
    assert header[25] == 1
    name, size, offset, next_extension = struct.unpack_from('<4I', data, header[26])
    assert data[header[4] + name:].split(b'\0', 1)[0] == b'aftershock.cook'
    assert size == 68 and next_extension == 0 and struct.unpack_from('<I', data, offset)[0] == 1
    copy = bytearray(data)
    claimed = copy[offset + 36:offset + 68]
    copy[offset + 36:offset + 68] = bytes(32)
    assert hashlib.sha256(copy).digest() == claimed
    return header


def check_model(path, animated, mirrored=False):
    data = path.read_bytes()
    assert data[:16] == b'INTERQUAKEMODEL\0'
    header = model_header(data)
    assert header[0] == 2 and header[1] == len(data)
    assert header[5] == 1 and header[8] == 3 and header[10] == 1
    assert header[13] == (2 if animated else 0)
    assert header[17] == (2 if animated else 0)
    arrays = [struct.unpack_from('<5I', data, header[9] + i * 20) for i in range(header[7])]
    positions = next(row for row in arrays if row[0] == 0)
    normals = next(row for row in arrays if row[0] == 2)
    assert positions[2:4] == normals[2:4] == (7, 3)
    expected_x = -1 if mirrored else 1
    coordinates = struct.unpack_from('<9f', data, positions[4])
    assert coordinates == (0, 0, 0, expected_x, 0, 2, -expected_x, 0, 2)
    face = struct.unpack_from('<3I', data, header[11])
    a, b, c = [coordinates[index * 3:index * 3 + 3] for index in face]
    # Clockwise face winding must still agree with its surface normal after a mirror.
    cross_y = (b[2] - a[2]) * (c[0] - a[0]) - (b[0] - a[0]) * (c[2] - a[2])
    assert cross_y > 0
    assert struct.unpack_from('<3f', data, normals[4]) == (0, -1, 0)
    if animated:
        names = []
        for i in range(2):
            name, first, count, fps, flags = struct.unpack_from('<IIIfI', data, header[18] + i * 20)
            names.append(data[header[4] + name:].split(b'\0', 1)[0].decode())
            assert count >= 2 and first + count <= header[19] and fps == 2
        assert names == ['idle', 'wave']


def check_audio(path):
    data = path.read_bytes()
    with wave.open(io.BytesIO(data)) as sound:
        assert (sound.getnchannels(), sound.getsampwidth(), sound.getframerate(), sound.getnframes()) == (1, 2, 22050, 1102)
        samples = struct.unpack('<1102h', sound.readframes(1102))
        assert max(samples) > 7000 and min(samples) < -7000
    assert data[36:40] == b'ASCK' and struct.unpack_from('<II', data, 40) == (68, 1)
    copy = bytearray(data)
    copy[80:112] = bytes(32)
    assert hashlib.sha256(copy).digest() == data[80:112]


def check_texture(path, vk_format, block_bytes):
    data = path.read_bytes()
    assert data[:12] == b'\xabKTX 20\xbb\r\n\x1a\n'
    fields = struct.unpack_from('<13I2Q', data, 12)
    assert fields[:9] == (vk_format, 1, 16, 16, 0, 0, 1, 5, 0)
    descriptor = data[fields[9]:fields[9] + fields[10]]
    assert descriptor[12] == {139: 131, 141: 132, 146: 134}[vk_format]
    assert descriptor[14] == (2 if vk_format == 146 else 1)
    offset, end = fields[11], fields[11] + fields[12]
    metadata = {}
    while offset < end:
        size, = struct.unpack_from('<I', data, offset)
        key, value = data[offset + 4:offset + 4 + size].split(b'\0', 1)
        metadata[key] = value
        if key == b'aftershock.contentHash':
            start = offset + 4 + len(key) + 1
            copy = bytearray(data)
            copy[start:start + 32] = bytes(32)
            assert hashlib.sha256(copy).digest() == value
        offset = (offset + 4 + size + 3) // 4 * 4
    assert metadata[b'aftershock.version'] == struct.pack('<I', 1)
    assert len(metadata[b'aftershock.sourceHash']) == len(metadata[b'aftershock.contentHash']) == 32
    for level in range(5):
        offset, size, raw = struct.unpack_from('<3Q', data, 80 + level * 24)
        side = max(1, 16 >> level)
        assert size == raw == ((side + 3) // 4) ** 2 * block_bytes
        assert offset % block_bytes == 0 and offset + size <= len(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-cook-tests'))
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'g++'))
    args = parser.parse_args()
    os.environ['CXX'] = args.cxx
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='aftershock-cook-', dir=args.output) as temporary:
        home = Path(temporary)
        project, names = source_assets(home)
        output = home / 'cooked'
        result = cook(project, output)
        assert sorted(result['built']) == sorted(names) and result['skipped'] == []
        for extension in ('wav', 'ogg'):
            check_audio(output / ('sounds/' + extension + '.wav'))
        index = (output / 'cook.index').read_bytes()
        assert (output / 'cook.revision').read_bytes() == hashlib.sha256(index).digest()
        magic, version, size, hashed = struct.unpack_from('<8sII32s', index)
        assert magic == b'ASIDX\0\0\0' and version == 1 and size == len(index) - 48
        assert hashed == hashlib.sha256(index[48:]).digest()
        count, = struct.unpack_from('<I', index, 48)
        assert len(index) == 52 + count * 104
        for row in range(count):
            path, expected, length, kind = struct.unpack_from('<64s32sII', index, 52 + row * 104)
            data = (output / path.rstrip(b'\0').decode()).read_bytes()
            assert len(data) == length and hashlib.sha256(data).digest() == expected and kind in (1, 2, 3)
        before = {}
        for name in names:
            manifest_path = output / (name + '.manifest.json')
            manifest = json.loads(manifest_path.read_text())
            assert manifest['version'] == 1 and manifest['name'] == name
            assert manifest['inputs'] and manifest['outputs']
            for item in manifest['inputs']:
                assert hashlib.sha256((home / item['path']).read_bytes()).hexdigest() == item['sha256']
            for item in manifest['outputs']:
                path = output / item['path']
                assert hashlib.sha256(path.read_bytes()).hexdigest() == item['sha256']
                before[item['path']] = (path.read_bytes(), path.stat().st_mtime_ns)
        for name in ('rig', 'packed', 'static', 'mirrored'):
            check_model(output / f'models/{name}.iqm', name in ('rig', 'packed'), name == 'mirrored')
        for fmt, native, size in [('bc7', 146, 16), ('bc5', 141, 16), ('bc4', 139, 8)]:
            check_texture(output / f'textures/{fmt}.ktx2', native, size)
        result = cook(project, output)
        assert result['built'] == [] and sorted(result['skipped']) == sorted(names)
        for name, (data, mtime) in before.items():
            assert (output / name).read_bytes() == data and (output / name).stat().st_mtime_ns == mtime
        second = home / 'second'
        cook(project, second)
        for name, (data, _) in before.items():
            assert (second / name).read_bytes() == data
        Image.new('RGBA', (16, 16), (48, 160, 224, 255)).save(home / 'color.png')
        result = cook(project, output)
        assert sorted(result['built']) == ['textures/bc4', 'textures/bc5', 'textures/bc7']
        assert sorted(result['skipped']) == ['models/mirrored', 'models/packed', 'models/rig', 'models/static']
        assert (output / 'textures/bc7.ktx2').read_bytes() != before['textures/bc7.ktx2'][0]
        (args.output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    with tempfile.TemporaryDirectory(prefix='aftershock-material-') as temporary:
        directory = Path(temporary)
        Image.new('RGBA', (16, 16), (255, 255, 255, 255)).save(directory / 'white.png')
        value = {'texture': 'white.png', 'baseColorFactor': [0.25, 0.5, 1, 0.5],
                 'doubleSided': True, 'unlit': True, 'alphaMode': 'BLEND'}
        (directory / 'paint.json').write_text(json.dumps(value))
        project = directory / 'assets.json'
        project.write_text(json.dumps({'version': 1, 'assets': [{'name': 'materials/paint', 'kind': 'material', 'source': 'paint.json'}]}))
        material_output = directory / 'cooked'
        assert cook(project, material_output)['built'] == ['materials/paint']
        payload = (material_output / 'materials/paint.asmat').read_bytes()[48:]
        assert struct.unpack_from('<4ffI', payload) == (0.25, 0.5, 1, 0.5, 0.5, 7)
        # Pillow's DDS decoder independently checks the BC7 texels after linear-factor baking.
        ktx = (material_output / 'materials/paint.ktx2').read_bytes()
        offset, length, _ = struct.unpack_from('<3Q', ktx, 80)
        dds = bytearray(148)
        dds[:4] = b'DDS '
        struct.pack_into('<7I', dds, 4, 124, 0x1007, 16, 16, 0, 0, 1)
        struct.pack_into('<II4s', dds, 76, 32, 4, b'DX10')
        struct.pack_into('<I', dds, 108, 0x1000)
        struct.pack_into('<5I', dds, 128, 99, 3, 0, 1, 0)
        import io
        decoded = Image.open(io.BytesIO(dds + ktx[offset:offset + length])).convert('RGBA')
        pixel = decoded.getpixel((8, 8))
        assert all(abs(actual - expected) <= 3 for actual, expected in zip(pixel, (137, 188, 255, 128))), pixel
        assert cook(project, material_output)['built'] == []
        revision = (material_output / 'cook.revision').read_bytes()
        with (args.output / 'watch.log').open('w+') as log:
            watcher = subprocess.Popen([sys.executable, 'tools/cook', str(project), '--output', str(material_output), '--watch'], cwd=ROOT, stdout=log, stderr=log)
            try:
                deadline = time.monotonic() + 15
                while '"skipped"' not in (args.output / 'watch.log').read_text():
                    assert watcher.poll() is None and time.monotonic() < deadline
                    time.sleep(0.05)
                Image.new('RGBA', (16, 16), (64, 128, 192, 255)).save(directory / 'white.png')
                deadline = time.monotonic() + 15
                while (material_output / 'cook.revision').read_bytes() == revision:
                    assert watcher.poll() is None and time.monotonic() < deadline
                    time.sleep(0.05)
                # Bad source edits keep the published revision and watcher alive.
                revision = (material_output / 'cook.revision').read_bytes()
                (directory / 'paint.json').write_text('{')
                deadline = time.monotonic() + 15
                while 'cook:' not in (args.output / 'watch.log').read_text():
                    assert watcher.poll() is None and time.monotonic() < deadline
                    time.sleep(0.05)
                assert (material_output / 'cook.revision').read_bytes() == revision
            finally:
                watcher.terminate()
                watcher.wait(timeout=10)
    fixture = ROOT / 'tests/assets/cook-character'
    provenance = json.loads((fixture / 'provenance.json').read_text())
    for name, expected in provenance['files'].items():
        assert hashlib.sha256((fixture / name).read_bytes()).hexdigest() == expected
    output = args.output / 'character'
    cook(fixture / 'assets.json', output)
    data = (output / 'models/character.iqm').read_bytes()
    header = model_header(data)
    assert (header[5], header[13], header[17], header[19]) == (6, 3, 2, 62)
    material = (output / 'models/character_material0.asmat').read_bytes()
    magic, version, size, hashed = struct.unpack_from('<8sII32s', material)
    assert magic == b'ASMAT\0\0\0' and version == 1 and size == len(material) - 48
    assert hashed == hashlib.sha256(material[48:]).digest()
    probe = args.output / 'model-probe'
    subprocess.run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
                    '-DUSE_VULKAN_API', '-Wall', '-Wextra', '-Werror', '-ffunction-sections', '-fdata-sections',
                    'tests/probes/cook_model.cpp', 'engine/qcommon/q_shared.cpp', 'engine/qcommon/q_math.cpp',
                    '-Wl,--gc-sections', '-o', str(probe)], cwd=ROOT, check=True)
    subprocess.run([str(probe), str(output / 'models/character.iqm')], check=True)
    material_probe = args.output / 'material-probe'
    subprocess.run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
                    '-DUSE_VULKAN_API', '-DAFTERSHOCK_DEVTOOLS', '-Wall', '-Wextra', '-Werror',
                    '-ffunction-sections', '-fdata-sections', 'tests/probes/cook_material.cpp',
                    'engine/qcommon/q_shared.cpp', 'engine/qcommon/q_math.cpp',
                    '-Wl,--gc-sections', '-o', str(material_probe)], cwd=ROOT, check=True)
    subprocess.run([str(material_probe)], check=True)
    sha_vendor = ROOT / 'third_party/sha256'
    for name, expected in json.loads((sha_vendor / 'provenance.json').read_text())['files'].items():
        assert hashlib.sha256((sha_vendor / name).read_bytes()).hexdigest() == expected
    sha_object = args.output / 'sha256.o'
    subprocess.run(['clang' if 'clang' in args.cxx else 'gcc', '-std=c99', '-O2', '-c',
                    str(sha_vendor / 'sha-256.c'), '-o', str(sha_object)], check=True)
    texture_probe = args.output / 'texture-probe'
    subprocess.run([*shlex.split(args.cxx), '-std=c++20', '-O2', '-fno-exceptions', '-fno-rtti',
                    '-Wall', '-Wextra', '-Werror', 'tests/probes/cook_texture.cpp',
                    'engine/render/tr_cooked.cpp', str(sha_object), '-o', str(texture_probe)], cwd=ROOT, check=True)
    subprocess.run([str(texture_probe), str(output / 'models/character_material0.ktx2'), str(output / 'models/character_material0.asmat')], check=True)
    print('PASS: static/skinned glTF/GLB, named clips, BC KTX2 mip chains, content hashes and incremental recook')


if __name__ == '__main__':
    main()
