#!/usr/bin/env python3
"""Render an owned glTF sphere and measure cooked PBR channel edits."""
import argparse
import json
import math
import os
from pathlib import Path
import shutil
import struct
import sys
import tempfile
import time

from PIL import Image

from cook import cook
from run import ROOT, content_maps
sys.path.insert(0, str(ROOT))
from tools.agent import Engine


def source(directory):
    blob, views, accessors = bytearray(), [], []

    def array(values, shape, components, fmt='f', component=5126):
        blob.extend(bytes(-len(blob) % 4))
        data = struct.pack('<' + fmt * len(values), *values)
        views.append({'buffer': 0, 'byteOffset': len(blob), 'byteLength': len(data)})
        blob.extend(data)
        accessors.append({'bufferView': len(views) - 1, 'componentType': component,
                          'count': len(values) // components, 'type': shape})
        return len(accessors) - 1

    positions, normals, tangents, uv, indices = [], [], [], [], []
    segments, rings = 24, 12
    for row in range(rings + 1):
        phi = math.pi * row / rings
        for column in range(segments + 1):
            theta = math.tau * column / segments
            normal = (math.sin(phi) * math.cos(theta), math.cos(phi), math.sin(phi) * math.sin(theta))
            positions.extend(normal)
            normals.extend(normal)
            tangents.extend((-math.sin(theta), 0, math.cos(theta), 1))
            uv.extend((column / segments, row / rings))
            if row < rings and column < segments:
                a = row * (segments + 1) + column
                b, c = a + 1, a + segments + 1
                indices.extend((a, b, c, b, c + 1, c))
    attributes = {'POSITION': array(positions, 'VEC3', 3), 'NORMAL': array(normals, 'VEC3', 3),
                  'TANGENT': array(tangents, 'VEC4', 4), 'TEXCOORD_0': array(uv, 'VEC2', 2)}
    accessors[attributes['POSITION']].update(min=[-1, -1, -1], max=[1, 1, 1])
    triangles = array(indices, 'SCALAR', 1, 'H', 5123)
    # The existing Animation tab previews clips; give the sphere an idle clip.
    times = array([0, 1], 'SCALAR', 1)
    accessors[times].update(min=[0], max=[1])
    translations = array([0, 0, 0, 0, 0, 0], 'VEC3', 3)
    document = {'asset': {'version': '2.0', 'generator': 'Aftershock owned PBR sphere'},
                'scene': 0, 'scenes': [{'nodes': [0]}], 'nodes': [{'mesh': 0}],
                'meshes': [{'primitives': [{'attributes': attributes, 'indices': triangles, 'material': 0}]}],
                'buffers': [{'uri': 'sphere.bin', 'byteLength': len(blob)}],
                'bufferViews': views, 'accessors': accessors,
                'animations': [{'name': 'idle', 'samplers': [{'input': times, 'output': translations}],
                                'channels': [{'sampler': 0, 'target': {'node': 0, 'path': 'translation'}}]}],
                'images': [{'uri': name + '.png'} for name in ('base', 'normal', 'mr', 'emissive')],
                'textures': [{'source': i} for i in range(4)],
                'materials': [{'pbrMetallicRoughness': {'baseColorTexture': {'index': 0},
                    'metallicRoughnessTexture': {'index': 2}, 'metallicFactor': 0, 'roughnessFactor': 0.7},
                    'normalTexture': {'index': 1}, 'emissiveTexture': {'index': 3},
                    'emissiveFactor': [0, 0, 0]}]}
    (directory / 'sphere.bin').write_bytes(blob)
    (directory / 'sphere.gltf').write_text(json.dumps(document))
    for name, color in [('base', (180, 130, 50, 128)), ('normal', (128, 128, 255, 255)),
                        ('mr', (255, 255, 255, 255)), ('emissive', (32, 180, 64, 255))]:
        Image.new('RGBA', (32, 32), color).save(directory / (name + '.png'))
    project = directory / 'assets.json'
    project.write_text(json.dumps({'version': 1, 'assets': [{'name': 'models/sphere', 'kind': 'model',
                       'source': 'sphere.gltf', 'material_model': 'metallic-roughness', 'scale': 32}]}))
    return project, document


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--content', choices=['quake3', 'openarena'], default='quake3')
    parser.add_argument('--data', type=Path, default=Path.home() / '.q3a/baseq3')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--ui', action='store_true', help='exercise shared and per-instance ImGui factor edits')
    args = parser.parse_args()
    temporary_output = tempfile.TemporaryDirectory(prefix='aftershock-material-output-', dir=os.environ.get('AFTERSHOCK_SCRATCH'))
    args.output = args.output.resolve() if args.output else Path(temporary_output.name)
    args.output.mkdir(parents=True, exist_ok=True)
    names = ['before', 'controls', 'shared', 'instance', 'cleared'] if args.ui else [
        'diffuse', 'metal', 'rough', 'normal_flat', 'normal', 'emissive', 'masked', 'blend', 'unlit', 'restored']
    with temporary_output, tempfile.TemporaryDirectory(prefix='aftershock-pbr-', dir=os.environ.get('AFTERSHOCK_SCRATCH')) as temporary:
        home = Path(temporary)
        base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
        base.mkdir()
        inputs = home / 'source'
        inputs.mkdir()
        project, document = source(inputs)
        cook(project, base)
        arguments = ['+set', 'r_gamma', '1', '+set', 'r_overBrightBits', '0', '+set', 'r_intensity', '1',
                     '+set', 'dev_reloadAssets', '1']
        with Engine(args.binary, args.data, args.content, home=home, arguments=arguments) as engine:
            engine.request('session', dt=8, seed=123)
            engine.request('map', name=content_maps(args.content)[0])
            engine.step(200)
            engine.request('panel', name='Animation')
            engine.request('animation.load', path='models/sphere.iqm')
            engine.step(3)
            materials = engine.request('assets', kind='materials', filter='models/sphere')['items']
            assert len(materials) == 1 and materials[0]['metallicRoughness'], materials
            for entry in materials:
                assert len(entry['stageInfo']) == entry['stages'], entry
                assert all(isinstance(stage['present'], bool) and len(stage['textures']) == 3
                           and isinstance(stage['stateBits'], int) for stage in entry['stageInfo']), entry
            models = engine.request('assets', kind='models', filter='models/sphere')['items']
            assert len(models) == 1 and models[0]['frames'] > 0, models
            engine.request('asset.select', kind='models', index=models[0]['index'])
            material_index = materials[0]['index']
            params = materials[0]['params']
            baseline = dict(params)
            material = document['materials'][0]
            pbr = material['pbrMetallicRoughness']
            for name in names:
                capture = engine.request('capture', name=name)
                engine.step(2)
                shutil.copyfile(base / capture['path'], args.output / (name + '.png'))
                preview = engine.request('editor.state')['animation']['viewport']
                if args.ui:
                    if name == 'before':
                        engine.request('panel', name='Materials')
                        engine.request('asset.select', kind='materials', index=material_index)
                    elif name == 'controls':
                        params = dict(baseline, metallic=1)
                        engine.request('material.set', index=material_index, **params)
                        engine.request('panel', name='Animation')
                    elif name == 'shared':
                        engine.request('material.preview', index=material_index, enabled=True)
                        engine.request('material.set', index=material_index, **baseline)
                    elif name == 'instance':
                        engine.request('material.preview', index=material_index, enabled=False)
                    engine.step(3)
                    continue
                if name == 'diffuse':
                    pbr['metallicFactor'] = 1
                elif name == 'metal':
                    pbr['roughnessFactor'] = 0.15
                elif name == 'rough':
                    pbr.update(metallicFactor=0, roughnessFactor=0.7)
                elif name == 'normal_flat':
                    # Opposite tangent-space Z gives a clear response even
                    # when a content set's preview sun lights the far side.
                    Image.new('RGBA', (32, 32), (128, 128, 0, 255)).save(inputs / 'normal.png')
                elif name == 'normal':
                    material['emissiveFactor'] = [0.5, 1, 0.5]
                elif name == 'emissive':
                    material.update(alphaMode='MASK', alphaCutoff=0.75)
                elif name == 'masked':
                    material['alphaMode'] = 'BLEND'
                elif name == 'blend':
                    material['alphaMode'] = 'OPAQUE'
                    material['emissiveFactor'] = [0, 0, 0]
                    pbr.update(metallicFactor=0, roughnessFactor=0.7)
                    Image.new('RGBA', (32, 32), (128, 128, 255, 255)).save(inputs / 'normal.png')
                    material['extensions'] = {'KHR_materials_unlit': {}}
                    document['extensionsUsed'] = ['KHR_materials_unlit']
                elif name == 'unlit':
                    del material['extensions']
                    del document['extensionsUsed']
                def reload_count():
                    return sum(item['reloads'] for kind in ('images', 'materials')
                               for item in engine.request('assets', kind=kind, filter='models/sphere')['items'])

                previous = reload_count()
                revision = (base / 'cook.revision').read_bytes()
                (inputs / 'sphere.gltf').write_text(json.dumps(document))
                cook(project, base)
                if (base / 'cook.revision').read_bytes() != revision:
                    # Offline hot reload polls real time; wait for its reported
                    # publication before sampling. Simulation still steps explicitly.
                    deadline = time.monotonic() + 10
                    while reload_count() <= previous:
                        assert time.monotonic() < deadline, 'cooked asset reload timed out'
                        engine.step(2)
                        time.sleep(0.01)
                engine.step(3)
            registry = engine.request('assets', kind='materials', filter='models/sphere')['items']
            if not args.ui:
                assert registry[0]['reloads'] >= 6, registry
            shutil.copyfile(engine.log_path, args.output / 'client.log')
        images = {name: Image.open(args.output / (name + '.png')).convert('RGB') for name in names}
        # The preview is drawn over the running map. Sample only the sphere's
        # interior, excluding that changing background, HUD and antialiased edge.
        x, y, width, height = preview
        center_x, center_y = x + width / 2, y + height / 2
        # This unit sphere is framed at four radii with a 40-degree vertical FOV.
        # Keep 90% of its projected radius, excluding the antialiased edge.
        radius = max(1, int(height * 0.9 / (8 * math.tan(math.radians(20)))))
        points = [(px, py) for px in range(int(center_x - radius), int(center_x + radius))
                  for py in range(int(center_y - radius), int(center_y + radius))
                  if (px - center_x) ** 2 + (py - center_y) ** 2 < radius ** 2]
        measurements = {}
        comparisons = [('before', 'shared'), ('shared', 'instance'), ('instance', 'cleared')] if args.ui else list(zip(names, names[1:]))
        for first, second in comparisons:
            changed = sum(max(abs(a - b) for a, b in zip(images[first].getpixel(point),
                          images[second].getpixel(point))) > 8 for point in points)
            assert changed > 50, (first, second, changed)
            measurements[first + ' -> ' + second] = changed
        equal = [('before', 'instance'), ('shared', 'cleared')] if args.ui else [('diffuse', 'restored')]
        for first, second in equal:
            assert all(images[first].getpixel(p) == images[second].getpixel(p) for p in points), (first, second, 'PBR factor round trip changed the stable preview')
        if not args.ui:
            assert all(max(abs(a - b) for a, b in zip(images['unlit'].getpixel(p), (180, 130, 50))) <= 3 for p in points), 'unlit glTF base color must retain its source sRGB appearance'
        (args.output / 'measurements.json').write_text(json.dumps(measurements, indent=2) + '\n')
        print('PASS: ImGui shared/instance material edits and isolated round trips' if args.ui else
              'PASS: owned glTF PBR sphere, metallic/roughness/normal/emission/mask/blend edits and exact round trip')


if __name__ == '__main__':
    main()
