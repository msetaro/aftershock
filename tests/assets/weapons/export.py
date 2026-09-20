#!/usr/bin/env python3
"""Explicit authoring of original GPL range props and synthetic one-shot sounds."""
import base64
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import wave

ROOT = Path(__file__).resolve().parent


def prop(name, boxes):
    data, views, accessors, meshes, nodes = bytearray(), [], [], [], []
    materials = [{'pbrMetallicRoughness': {'baseColorFactor': color}}
                 for color in ([0.12, 0.15, 0.18, 1], [0.2, 0.45, 0.1, 1], [0.1, 0.6, 0.9, 1])]

    def array(values, fmt, kind, shape, count):
        data.extend(bytes(-len(data) % 4))
        encoded = struct.pack('<' + fmt * len(values), *values)
        views.append({'buffer': 0, 'byteOffset': len(data), 'byteLength': len(encoded)})
        data.extend(encoded)
        accessors.append({'bufferView': len(views) - 1, 'componentType': kind, 'type': shape, 'count': count})
        return len(accessors) - 1

    for center, half, material in boxes:
        positions, normals, uv, indices = [], [], [], []
        for axis, sign, u, v in [(0, 1, 1, 2), (0, -1, 2, 1), (1, 1, 2, 0),
                                 (1, -1, 0, 2), (2, 1, 0, 1), (2, -1, 1, 0)]:
            first = len(positions) // 3
            for a, b in [(-1, -1), (1, -1), (1, 1), (-1, 1)]:
                p, n = list(center), [0, 0, 0]
                p[axis] += sign * half[axis]
                p[u] += a * half[u]
                p[v] += b * half[v]
                n[axis] = sign
                positions.extend(p)
                normals.extend(n)
                uv.extend([(a + 1) / 2, (b + 1) / 2])
            indices.extend(first + i for i in (0, 1, 2, 0, 2, 3))
        position = array(positions, 'f', 5126, 'VEC3', 24)
        accessors[position].update(min=[min(positions[i::3]) for i in range(3)],
                                   max=[max(positions[i::3]) for i in range(3)])
        attributes = {'POSITION': position, 'NORMAL': array(normals, 'f', 5126, 'VEC3', 24),
                      'TEXCOORD_0': array(uv, 'f', 5126, 'VEC2', 24)}
        meshes.append({'primitives': [{'attributes': attributes,
            'indices': array(indices, 'H', 5123, 'SCALAR', 36), 'material': material}]})
        nodes.append({'mesh': len(meshes) - 1})
    document = {'asset': {'version': '2.0', 'generator': 'Aftershock original range prop authoring'},
        'scene': 0, 'scenes': [{'nodes': list(range(len(nodes)))}], 'nodes': nodes,
        'meshes': meshes, 'materials': materials, 'bufferViews': views, 'accessors': accessors,
        'buffers': [{'byteLength': len(data), 'uri': 'data:application/octet-stream;base64,' + base64.b64encode(data).decode()}]}
    (ROOT / (name + '.gltf')).write_text(json.dumps(document, indent=2) + '\n')


# Optic bone faces along local Z; the ring lies in local XY. Convert that
# socket-local frame to glTF Y-up before the cooker converts it back.
optic = [([0, 0.08, 0], [0.08, 0.012, 0.035], 0),
         ([0, -0.08, 0], [0.08, 0.012, 0.035], 0),
         ([-0.08, 0, 0], [0.012, 0.08, 0.035], 0),
         ([0.08, 0, 0], [0.012, 0.08, 0.035], 0),
         ([0, 0, 0.02], [0.008, 0.008, 0.004], 2)]
prop('optic', [([c[0], c[2], -c[1]], [h[0], h[2], h[1]], m) for c, h, m in optic])
# New range variant removes the old solid placeholder sight, retaining the
# accepted rig/animations/buffer bytes and the socket for data attachments.
rifle = json.loads((ROOT.parent / 'animation/rifle.gltf').read_text())
placeholder = next(i for i, node in enumerate(rifle['nodes']) if node.get('name') == 'optic_mesh')
for node in rifle['nodes']:
    if 'children' in node:
        node['children'] = [i for i in node['children'] if i != placeholder]
for scene in rifle['scenes']:
    scene['nodes'] = [i for i in scene['nodes'] if i != placeholder]
rifle['buffers'][0]['uri'] = '../animation/rifle.bin'
(ROOT / 'rifle.gltf').write_text(json.dumps(rifle, indent=2) + '\n')
prop('grenade', [([0, 0, 0], [0.07, 0.09, 0.07], 1),
                 ([0, 0.105, 0], [0.04, 0.02, 0.04], 0),
                 ([0.05, 0.03, 0], [0.012, 0.1, 0.025], 0)])
for name, duration, tone, noise in [('shot', 0.16, 95, 0.8), ('reload', 0.09, 1800, 0.25)]:
    rng = random.Random(11)
    samples = []
    for i in range(int(duration * 48000)):
        t = i / 48000
        envelope = (1 - t / duration) ** 3 * min(1, t * 2000)
        value = (noise * rng.uniform(-1, 1) + (1 - noise) * math.sin(t * tone * math.tau)) * envelope
        samples.append(round(value * 28000))
    with wave.open(str(ROOT / (name + '.wav')), 'wb') as output:
        output.setparams((1, 2, 48000, 0, 'NONE', 'not compressed'))
        output.writeframes(struct.pack('<' + 'h' * len(samples), *samples))
files = ['export.py', 'optic.gltf', 'rifle.gltf', 'grenade.gltf', 'shot.wav', 'reload.wav']
(ROOT / 'provenance.json').write_text(json.dumps({'license': 'GPL-2.0-or-later',
    'origin': 'Original procedural box props and synthesized sound; no external game assets.',
    'files': {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in files}}, indent=2) + '\n')
