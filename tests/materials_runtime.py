#!/usr/bin/env python3
"""Render an owned glTF sphere and measure cooked PBR channel edits."""
import argparse
import json
import math
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

from PIL import Image

from cook import cook
from run import ROOT, content_maps, content_settings
from window import XInput, wait_for


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
    parser.add_argument('--output', type=Path, default=Path('/tmp/aftershock-material-runtime'))
    parser.add_argument('--ui', action='store_true', help='exercise shared and per-instance ImGui factor edits')
    parser.add_argument('--inside-xvfb', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if not args.inside_xvfb:
        subprocess.run(['timeout', '150', 'xvfb-run', '-a', sys.executable, str(Path(__file__).resolve()),
                        *sys.argv[1:], '--inside-xvfb'], cwd=ROOT, check=True)
        return
    args.output.mkdir(parents=True, exist_ok=True)
    paks = sorted(args.data.resolve().glob('*.pk3'))
    icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
    if not paks or len(icds) != 1:
        parser.error('installed content and one lavapipe ICD are required')
    names = ['before', 'controls', 'shared', 'instance', 'cleared'] if args.ui else [
        'diffuse', 'metal', 'rough', 'normal_flat', 'normal', 'emissive', 'masked', 'blend', 'unlit', 'restored']
    with tempfile.TemporaryDirectory(prefix='aftershock-pbr-') as temporary:
        home = Path(temporary)
        base = home / ('baseoa' if args.content == 'openarena' else 'baseq3')
        base.mkdir()
        for pak in paks:
            (base / pak.name).symlink_to(pak)
        inputs = home / 'source'
        inputs.mkdir()
        project, document = source(inputs)
        cook(project, base)
        commands = [f'devmap {content_maps(args.content)[0]}', 'wait 10', 'set timescale 0',
                    'set dev_reloadAssets 1', 'dev_reloadAssets', 'echo pbr_ready', 'wait 240']
        for name in names:
            commands += ['screenshot ' + name, 'wait 3', 'echo pbr_' + name, 'wait 240' if args.ui else 'wait 100']
        commands += ['devtools_status', 'quit']
        (base / 'pbr.cfg').write_text('\n'.join(commands) + '\n')
        env = dict(os.environ, LP_NUM_THREADS='1', VK_DRIVER_FILES=str(icds[0]), VK_ICD_FILENAMES=str(icds[0]))
        command = [str(args.binary.resolve()), '+set', 'fs_basepath', str(home), '+set', 'fs_homepath', str(home),
                   *content_settings(args.content), '+set', 'net_enabled', '0', '+set', 'sv_pure', '0',
                   '+set', 'r_mode', '3', '+set', 'r_fullscreen', '0', '+set', 's_initsound', '0',
                   '+set', 'r_gamma', '1', '+set', 'r_overBrightBits', '0', '+set', 'r_intensity', '1',
                   '+set', 'dev_tools', '1', '+set', 'dev_reloadAssets', '1', '+set', 'com_maxfps', '20',
                   '+set', 'cl_autoRecordDemo', '0', '+exec', 'pbr.cfg']
        log_path = args.output / 'client.log'
        with log_path.open('wb') as log:
            process = subprocess.Popen(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
            device = XInput()
            try:
                wait_for(lambda: b'pbr_ready' in log_path.read_bytes(), process)
                device.verify_window(process)
                device.click(440, 64)
                device.click(120, 91)
                for char in 'models/sphere.iqm':
                    device.key({'/': 'slash', '.': 'period'}.get(char, char))
                device.click(80, 137)
                material = document['materials'][0]
                pbr = material['pbrMetallicRoughness']
                for name in names:
                    wait_for(lambda: ('pbr_' + name).encode() in log_path.read_bytes().splitlines(), process)
                    shutil.copyfile(base / ('screenshots/' + name + '.tga'), args.output / (name + '.tga'))
                    if args.ui:
                        if name == 'before':
                            device.click(235, 64)
                            device.click(120, 87)
                            for char in 'models/sphere':
                                device.key('slash' if char == '/' else char)
                            device.click(100, 116)
                        elif name == 'controls':
                            device.click(380, 371)
                            device.click(440, 64)
                        elif name == 'shared':
                            device.click(235, 64)
                            device.click(26, 309)
                            device.click(25, 371)
                            device.click(440, 64)
                        elif name == 'instance':
                            device.click(235, 64)
                            device.click(26, 309)
                            device.click(440, 64)
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
                    (inputs / 'sphere.gltf').write_text(json.dumps(document))
                    cook(project, base)
                process.wait(timeout=15)
                assert process.returncode == 0
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
                device.close()
        text = log_path.read_text()
        assert not any(s in text for s in ('ERROR:', 'Signal caught', 'Invalid or unavailable', 'Invalid or unsupported'))
        if not args.ui:
            assert text.count('Cooked material reloaded:') >= 6
        images = {name: Image.open(args.output / (name + '.tga')).convert('RGB') for name in names}
        # The preview is drawn over the running map. Sample only the sphere's
        # interior, excluding that changing background, HUD and antialiased edge.
        points = [(x, y) for x in range(278, 363) for y in range(354, 439)
                  if (x - 320) ** 2 + (y - 396) ** 2 < 42 ** 2]
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
