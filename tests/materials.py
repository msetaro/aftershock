#!/usr/bin/env python3
"""Cook an owned glTF metallic/roughness material without changing legacy assets."""
import hashlib
import io
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile

from PIL import Image

from cook import cook, source_assets
from run import ROOT


def decoded(path, level=0):
    data = path.read_bytes()
    vk_format, _, width, height = struct.unpack_from('<4I', data, 12)
    assert vk_format in (145, 146)
    width, height = max(1, width >> level), max(1, height >> level)
    offset, length, _ = struct.unpack_from('<3Q', data, 80 + level * 24)
    dds = bytearray(148)
    dds[:4] = b'DDS '
    struct.pack_into('<7I', dds, 4, 124, 0x1007, height, width, 0, 0, 1)
    struct.pack_into('<II4s', dds, 76, 32, 4, b'DX10')
    struct.pack_into('<I', dds, 108, 0x1000)
    struct.pack_into('<5I', dds, 128, 99 if vk_format == 146 else 98, 3, 0, 1, 0)
    return Image.open(io.BytesIO(dds + data[offset:offset + length])).convert('RGBA'), vk_format


def source(directory):
    project, _ = source_assets(directory)
    document = json.loads((directory / 'tangent.gltf').read_text())
    colors = {'base': (160, 80, 40, 192), 'normal': (128, 128, 255, 255),
              'mr': (23, 96, 200, 255), 'emissive': (32, 64, 128, 255)}
    for name, color in colors.items():
        Image.new('RGBA', (16, 16), color).save(directory / (name + '.png'))
    document['images'] = [{'uri': name + '.png'} for name in colors]
    document['textures'] = [{'source': i} for i in range(4)]
    document['materials'] = [{
        'pbrMetallicRoughness': {'baseColorTexture': {'index': 0},
                                'metallicRoughnessTexture': {'index': 2},
                                'baseColorFactor': [0.5, 0.75, 1, 0.5],
                                'metallicFactor': 0.75, 'roughnessFactor': 0.5},
        'normalTexture': {'index': 1, 'scale': 0.75},
        'emissiveTexture': {'index': 3}, 'emissiveFactor': [0.25, 0.5, 1],
        'alphaMode': 'MASK', 'alphaCutoff': 0.25, 'doubleSided': True}]
    document['meshes'][0]['primitives'][0]['material'] = 0
    (directory / 'pbr.gltf').write_text(json.dumps(document))
    project.write_text(json.dumps({'version': 1, 'assets': [
        {'name': 'models/pbr', 'kind': 'model', 'source': 'pbr.gltf',
         'material_model': 'metallic-roughness', 'scale': 1}]}))
    return project, document


def close(actual, expected, tolerance=3):
    assert all(abs(a - b) <= tolerance for a, b in zip(actual, expected)), (actual, expected)


def main():
    with tempfile.TemporaryDirectory(prefix='aftershock-materials-') as temporary:
        directory = Path(temporary)
        probe = directory / 'material-probe'
        sha = directory / 'sha.o'
        subprocess.run([*shlex.split(os.environ.get('CC', 'gcc')), '-std=c99', '-O2', '-c',
                        'third_party/sha256/sha-256.c', '-o', str(sha)], cwd=ROOT, check=True)
        subprocess.run([*shlex.split(os.environ.get('CXX', 'g++')), '-std=c++20', '-O2',
                        '-fno-exceptions', '-fno-rtti', '-Wall', '-Wextra', '-Werror',
                        '-ffunction-sections', '-fdata-sections', 'tests/probes/materials.cpp',
                        'engine/render/tr_cooked.cpp', str(sha),
                        '-Wl,--gc-sections', '-o', str(probe)], cwd=ROOT, check=True)
        project, document = source(directory)
        output = directory / 'cooked'
        assert cook(project, output)['built'] == ['models/pbr']
        data = (output / 'models/pbr_material0.asmat').read_bytes()
        magic, version, size, digest = struct.unpack_from('<8sII32s', data)
        assert magic == b'ASMAT\0\0\0' and version == 2, 'PBR requires the version-2 material contract'
        assert size == 240 and len(data) == 48 + size
        assert hashlib.sha256(data[48:]).digest() == digest
        subprocess.run([str(probe), str(output / 'models/pbr_material0.asmat')], check=True)
        values = struct.unpack_from('<11fI', data, 48)
        assert values == (0.5, 0.75, 1, 0.5, 0.25, 0.5, 1, 0.75, 0.5, 0.75, 0.25, 9)
        paths = [data[96 + i * 64:160 + i * 64].split(b'\0', 1)[0].decode() for i in range(3)]
        # Factors stay editable; textures contain the source values, not baked factors.
        for path, expected, vk_format in zip(paths, [(160, 80, 40, 192),
                                                    (128, 128, 255, 96),
                                                    (32, 64, 128, 200)], [146, 145, 146]):
            for level in (0, 4):
                image, actual_format = decoded(output / path, level)
                assert actual_format == vk_format
                close(image.getpixel((0, 0)), expected)
        manifest = json.loads((output / 'models/pbr.manifest.json').read_text())
        assert {row['path'] for row in manifest['inputs']} == {
            'pbr.gltf', 'rig.bin', 'base.png', 'normal.png', 'mr.png', 'emissive.png'}
        assert cook(project, output)['built'] == []
        standalone = json.loads(json.dumps(document['materials'][0]))
        for parent, key in ((standalone['pbrMetallicRoughness'], 'baseColorTexture'),
                            (standalone['pbrMetallicRoughness'], 'metallicRoughnessTexture'),
                            (standalone, 'normalTexture'), (standalone, 'emissiveTexture')):
            info = parent[key]
            info['uri'] = document['images'][info.pop('index')]['uri']
        (directory / 'paint.json').write_text(json.dumps(standalone))
        direct = directory / 'direct.json'
        direct.write_text(json.dumps({'version': 1, 'assets': [
            {'name': 'materials/paint', 'kind': 'material', 'source': 'paint.json',
             'material_model': 'metallic-roughness'}]}))
        assert cook(direct, output)['built'] == ['materials/paint']
        assert (output / 'materials/paint.asmat').read_bytes()[48:96] == data[48:96]
        before = {path: (output / path).read_bytes() for path in paths}
        # Metallic data must not weight emissive mip filtering like opacity would.
        mr = Image.new('RGBA', (16, 16), (23, 96, 0, 255))
        for x in range(8, 16):
            for y in range(16):
                mr.putpixel((x, y), (23, 96, 255, 255))
        mr.save(directory / 'mr.png')
        assert cook(project, output)['built'] == ['models/pbr']
        image, _ = decoded(output / paths[2], 4)
        close(image.getpixel((0, 0)), (32, 64, 128, 128))
        assert (output / paths[0]).read_bytes() == before[paths[0]]
        # The opt-in recipe does not alter the existing version-1 compatibility path.
        document['materials'][0]['alphaCutoff'] = 0.5
        (directory / 'pbr.gltf').write_text(json.dumps(document))
        definition = json.loads(project.read_text())
        del definition['assets'][0]['material_model']
        project.write_text(json.dumps(definition))
        cook(project, output)
        legacy = (output / 'models/pbr_material0.asmat').read_bytes()
        assert struct.unpack_from('<II', legacy, 8) == (1, 88)
        print('PASS: PBR factors, channel/color-space packing, data mips, dependencies and legacy material')


if __name__ == '__main__':
    main()
