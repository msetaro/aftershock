#!/usr/bin/env python3
"""Cook ordinary static/skinned source assets and check incremental, portable outputs."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile

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
    Image.new('RGBA', (16, 16), (128, 96, 255, 255)).save(directory / 'color.png')
    assets = [{'name': 'models/' + name, 'kind': 'model', 'source': source, 'scale': 1, 'fps': 2}
              for name, source in [('rig', 'rig.gltf'), ('packed', 'rig.glb'), ('static', 'static.gltf')]]
    assets += [{'name': 'textures/' + fmt, 'kind': 'texture', 'source': 'color.png',
                'format': fmt, 'srgb': fmt == 'bc7'} for fmt in ('bc7', 'bc5', 'bc4')]
    project = directory / 'assets.json'
    project.write_text(json.dumps({'version': 1, 'assets': assets}))
    return project, [a['name'] for a in assets]


def cook(project, output):
    command = ['python3', 'tools/cook', str(project), '--output', str(output)]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError('cooker failed:\n' + result.stdout + result.stderr)
    return json.loads(result.stdout)


def check_model(path, animated):
    data = path.read_bytes()
    assert data[:16] == b'INTERQUAKEMODEL\0'
    header = struct.unpack_from('<27I', data, 16)
    assert header[0] == 2 and header[1] == len(data)
    assert header[5] == 1 and header[8] == 3 and header[10] == 1
    assert header[13] == (2 if animated else 0)
    assert header[17] == (2 if animated else 0)
    arrays = [struct.unpack_from('<5I', data, header[9] + i * 20) for i in range(header[7])]
    positions = next(row for row in arrays if row[0] == 0)
    normals = next(row for row in arrays if row[0] == 2)
    assert positions[2:4] == normals[2:4] == (7, 3)
    assert struct.unpack_from('<9f', data, positions[4]) == (0, 0, 0, 1, 0, 2, -1, 0, 2)
    assert struct.unpack_from('<3f', data, normals[4]) == (0, -1, 0)
    if animated:
        names = []
        for i in range(2):
            name, first, count, fps, flags = struct.unpack_from('<IIIfI', data, header[18] + i * 20)
            names.append(data[header[4] + name:].split(b'\0', 1)[0].decode())
            assert count >= 2 and first + count <= header[19] and fps == 2
        assert names == ['idle', 'wave']


def check_texture(path, vk_format, block_bytes):
    data = path.read_bytes()
    assert data[:12] == b'\xabKTX 20\xbb\r\n\x1a\n'
    fields = struct.unpack_from('<13I2Q', data, 12)
    assert fields[:9] == (vk_format, 1, 16, 16, 0, 0, 1, 5, 0)
    for level in range(5):
        offset, size, raw = struct.unpack_from('<3Q', data, 80 + level * 24)
        side = max(1, 16 >> level)
        assert size == raw == ((side + 3) // 4) ** 2 * block_bytes
        assert offset % block_bytes == 0 and offset + size <= len(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-cook-tests'))
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='aftershock-cook-', dir=args.output) as temporary:
        home = Path(temporary)
        project, names = source_assets(home)
        output = home / 'cooked'
        result = cook(project, output)
        assert sorted(result['built']) == sorted(names) and result['skipped'] == []
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
        for name in ('rig', 'packed', 'static'):
            check_model(output / f'models/{name}.iqm', name != 'static')
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
        assert sorted(result['skipped']) == ['models/packed', 'models/rig', 'models/static']
        assert (output / 'textures/bc7.ktx2').read_bytes() != before['textures/bc7.ktx2'][0]
        (args.output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: static/skinned glTF/GLB, named clips, BC KTX2 mip chains, content hashes and incremental recook')


if __name__ == '__main__':
    main()
