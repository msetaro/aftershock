"""Cook glTF scenes into the engine's existing IQM v2 payload."""
import bisect
import hashlib
import io
import json
import math
import struct

from PIL import Image

from gltf import Document, IDENTITY, cross, decompose, dot, inverse, mul, normal, point, slerp, trs, unit
import texture


def wrapped(magic, payload, version=1):
    return struct.pack('<8sII32s', magic, version, len(payload), hashlib.sha256(payload).digest()) + payload


def material_bytes(material, image_name):
    pbr = material.get('pbrMetallicRoughness', {})
    color = pbr.get('baseColorFactor', [1, 1, 1, 1])
    if len(color) != 4 or any(not math.isfinite(v) or not 0 <= v <= 1 for v in color):
        raise ValueError('base color factors must be four finite values in [0,1]')
    mode = material.get('alphaMode', 'OPAQUE')
    if mode not in ('OPAQUE', 'MASK', 'BLEND'):
        raise ValueError('unsupported material alpha mode')
    flags = int(material.get('doubleSided', False)) | (2 if 'KHR_materials_unlit' in material.get('extensions', {}) else 0)
    flags |= 4 if mode == 'BLEND' else 8 if mode == 'MASK' else 0
    path = image_name.encode()
    if len(path) >= 64:
        raise ValueError('cooked material texture path exceeds the engine qpath limit')
    cutoff = material.get('alphaCutoff', 0.5)
    if not math.isfinite(cutoff) or not 0 <= cutoff <= 1:
        raise ValueError('material alpha cutoff must be in [0,1]')
    if mode == 'MASK' and cutoff != 0.5:
        raise ValueError('the current native material path supports MASK cutoff 0.5; arbitrary cutoffs belong to #13')
    return wrapped(b'ASMAT\0\0\0', struct.pack('<4ffI64s', *color, cutoff, flags, path))


def material_texture(raw, value):
    options = {'format': 'bc7', 'srgb': True}
    color = value.get('pbrMetallicRoughness', {}).get('baseColorFactor', [1, 1, 1, 1])
    if color != [1, 1, 1, 1]:
        options['color_factor'] = color
    if value.get('alphaMode', 'OPAQUE') == 'OPAQUE':
        options['opaque'] = True
    return texture.cook(raw, options)


def pbr_material(value, name, image):
    """Three BC7 textures; keep factors separate for native instance/live edits."""
    pbr = value.get('pbrMetallicRoughness', {})
    color = pbr.get('baseColorFactor', [1, 1, 1, 1])
    emissive = value.get('emissiveFactor', [0, 0, 0])
    metallic, roughness = pbr.get('metallicFactor', 1), pbr.get('roughnessFactor', 1)
    normal_scale = value.get('normalTexture', {}).get('scale', 1)
    cutoff = value.get('alphaCutoff', 0.5)
    if (len(color) != 4 or len(emissive) != 3 or
            any(not math.isfinite(v) or not 0 <= v <= 1 for v in [*color, *emissive, metallic, roughness, cutoff]) or
            not math.isfinite(normal_scale)):
        raise ValueError('PBR factors must be finite; color/emissive/metallic/roughness/cutoff must be in [0,1]')
    mode = value.get('alphaMode', 'OPAQUE')
    if mode not in ('OPAQUE', 'MASK', 'BLEND'):
        raise ValueError('unsupported PBR alpha mode')
    if set(value.get('extensions', {})) - {'KHR_materials_unlit'} or pbr.get('extensions'):
        raise ValueError('bake unsupported PBR material extensions before cooking')
    if value.get('occlusionTexture'):
        raise ValueError('bake occlusion into the lighting; this material contract has no occlusion channel')
    flags = int(value.get('doubleSided', False)) | (2 if 'KHR_materials_unlit' in value.get('extensions', {}) else 0)
    flags |= 4 if mode == 'BLEND' else 8 if mode == 'MASK' else 0

    def channel(info, default):
        if not info:
            return Image.new('RGBA', (1, 1), default)
        if info.get('texCoord', 0) != 0 or info.get('extensions'):
            raise ValueError('bake PBR texture transforms/additional UV sets before cooking')
        with Image.open(io.BytesIO(image(info))) as opened:
            if not 1 <= opened.width <= 16384 or not 1 <= opened.height <= 16384:
                raise ValueError('PBR texture dimensions must be between 1 and 16384')
            return opened.convert('RGBA')

    base = channel(pbr.get('baseColorTexture'), (255, 255, 255, 255))
    normal_map = channel(value.get('normalTexture'), (128, 128, 255, 255))
    mr = channel(pbr.get('metallicRoughnessTexture'), (255, 255, 255, 255))
    emission = channel(value.get('emissiveTexture'), (255, 255, 255, 255))

    def pack(rgb, alpha):
        # A constant missing channel can expand losslessly. Artists bake other
        # resolution mismatches explicitly, avoiding silently resampled normals.
        if rgb.size != alpha.size:
            if rgb.size == (1, 1):
                rgb = rgb.resize(alpha.size, Image.Resampling.NEAREST)
            elif alpha.size == (1, 1):
                alpha = alpha.resize(rgb.size, Image.Resampling.NEAREST)
            else:
                raise ValueError('packed PBR texture channels must have equal dimensions (or be constant 1x1)')
        result = rgb.copy()
        result.putalpha(alpha)
        return result

    packed = (base, pack(normal_map, mr.getchannel('G')), pack(emission, mr.getchannel('B')))
    paths = [name + suffix + '.ktx2' for suffix in ('_b', '_n', '_e')]
    if any(len(path.encode()) >= 64 for path in paths):
        raise ValueError('PBR texture path exceeds the 63-byte engine limit')
    payload = struct.pack('<11fI', *color, *emissive, metallic, roughness, normal_scale, cutoff, flags)
    payload += b''.join(struct.pack('<64s', path.encode()) for path in paths)
    outputs = {name + '.asmat': wrapped(b'ASMAT\0\0\0', payload, version=2)}
    for index, (path, pixels) in enumerate(zip(paths, packed)):
        stream = io.BytesIO()
        pixels.save(stream, format='PNG')
        outputs[path] = texture.cook(stream.getvalue(), {'format': 'bc7', 'srgb': index != 1,
                                                        'normal': index == 1, 'data_alpha': index != 0})
    return outputs


def cook_material(path, name, read, options=None):
    value = json.loads(read(path))
    workflow = (options or {}).get('material_model', 'legacy')
    if workflow == 'metallic-roughness':
        return pbr_material(value, name, lambda info: read(path.parent / info['uri']))
    if workflow != 'legacy':
        raise ValueError('material_model must be legacy or metallic-roughness')
    material = {key: value[key] for key in ('alphaMode', 'alphaCutoff', 'doubleSided') if key in value}
    material['pbrMetallicRoughness'] = {'baseColorFactor': value.get('baseColorFactor', [1, 1, 1, 1])}
    if value.get('unlit', False):
        material['extensions'] = {'KHR_materials_unlit': {}}
    image_name = name + '.ktx2'
    payload = material_bytes(material, image_name)
    if value.get('texture'):
        raw = read(path.parent / value['texture'])
    else:
        stream = io.BytesIO()
        Image.new('RGBA', (1, 1), (255, 255, 255, 255)).save(stream, format='PNG')
        raw = stream.getvalue()
    return {name + '.asmat': payload, image_name: material_texture(raw, material)}


def cook(path, name, options, read):
    doc = Document(path, read)
    source = doc.data
    workflow = options.get('material_model', 'legacy')
    if workflow not in ('legacy', 'metallic-roughness'):
        raise ValueError('material_model must be legacy or metallic-roughness')
    nodes = source.get('nodes', [])
    scale, fps = float(options.get('scale', 32)), float(options.get('fps', 30))
    if not math.isfinite(scale) or scale <= 0 or not math.isfinite(fps) or not 1 <= fps <= 240:
        raise ValueError('model scale must be positive and fps must be in [1,240]')
    basis = [scale, 0, 0, 0, 0, 0, -scale, 0, 0, scale, 0, 0, 0, 0, 0, 1]
    inverse_basis = inverse(basis)
    parents = [-1] * len(nodes)
    for index, node in enumerate(nodes):
        for child in node.get('children', []):
            if not 0 <= child < len(nodes) or parents[child] != -1:
                raise ValueError('glTF nodes must form trees')
            parents[child] = index
    roots = source.get('scenes', [{}])[source.get('scene', 0)].get('nodes',
                       [i for i, parent in enumerate(parents) if parent == -1])
    active = []

    def visit(index, chain):
        if index in chain or len(chain) > 256 or not 0 <= index < len(nodes):
            raise ValueError('invalid or excessively deep glTF scene tree')
        active.append(index)
        for child in nodes[index].get('children', []):
            visit(child, [*chain, index])

    for root in roots:
        visit(root, [])
    mesh_nodes = [i for i in active if 'mesh' in nodes[i]]
    if not mesh_nodes:
        raise ValueError('the selected glTF scene has no meshes')
    defaults = []
    for node in nodes:
        if 'matrix' in node:
            matrix = node['matrix']
            defaults.append(decompose([matrix[c * 4 + r] for r in range(4) for c in range(4)]))
        else:
            defaults.append((node.get('translation', [0, 0, 0]), node.get('rotation', [0, 0, 0, 1]), node.get('scale', [1, 1, 1])))

    def worlds(overrides=None):
        result = {}
        visiting = set()

        def world(index):
            if index in result:
                return result[index]
            if index in visiting:
                raise ValueError('cyclic glTF transform hierarchy')
            visiting.add(index)
            t, r, s = defaults[index]
            changed = (overrides or {}).get(index, {})
            value = trs(changed.get('translation', t), changed.get('rotation', r), changed.get('scale', s))
            if parents[index] != -1:
                value = mul(world(parents[index]), value)
            result[index] = value
            visiting.remove(index)
            return value

        for index in active:
            world(index)
        return result

    rest = worlds()
    joints, skin_maps = [], {}
    skin_ids = sorted({nodes[i]['skin'] for i in mesh_nodes if 'skin' in nodes[i]})
    for skin_id in skin_ids:
        skin = source['skins'][skin_id]
        indices = skin['joints']
        matrices = doc.accessor(skin['inverseBindMatrices']) if 'inverseBindMatrices' in skin else [IDENTITY] * len(indices)
        if len(matrices) != len(indices) or len(set(indices)) != len(indices):
            raise ValueError('skin joint/bind matrix counts do not agree')
        mapping = {}
        remaining = list(range(len(indices)))
        while remaining:
            progressed = False
            for slot in remaining[:]:
                node_id = indices[slot]
                ancestor = parents[node_id]
                while ancestor != -1 and ancestor not in indices:
                    ancestor = parents[ancestor]
                if ancestor != -1 and ancestor not in mapping:
                    continue
                raw = matrices[slot]
                ibm = [raw[c * 4 + r] for r in range(4) for c in range(4)]
                bind = mul(mul(basis, inverse(ibm)), inverse_basis)
                mapping[node_id] = len(joints)
                joint_name = nodes[node_id].get('name', 'joint' + str(node_id))
                if len(skin_ids) > 1:
                    joint_name = f'skin{skin_id}/' + joint_name
                joints.append({'node': node_id, 'parent': mapping[ancestor] if ancestor != -1 else -1,
                               'bind': bind, 'name': joint_name})
                remaining.remove(slot)
                progressed = True
            if not progressed:
                raise ValueError('skin joint hierarchy cannot be ordered')
        skin_maps[skin_id] = [mapping[index] for index in indices]
    # Static objects in an animated/mixed scene use a rigid joint; static-only scenes remain joint-free.
    rigid = {}
    if joints or source.get('animations'):
        for node_id in mesh_nodes:
            if 'skin' not in nodes[node_id]:
                rigid[node_id] = len(joints)
                joints.append({'node': node_id, 'parent': -1, 'bind': IDENTITY.copy(),
                               'name': nodes[node_id].get('name', 'object' + str(node_id))})
    if len(joints) > 128:
        raise ValueError('the cooked model exceeds the existing 128-joint engine limit')
    for joint in joints:
        parent_bind = joints[joint['parent']]['bind'] if joint['parent'] >= 0 else IDENTITY
        joint['local'] = decompose(mul(inverse(parent_bind), joint['bind']))

    frames, clips = [], []
    clip_names = set()
    if len(source.get('animations', [])) > 4096:
        raise ValueError('native IQM supports at most 4096 clips')
    for animation in source.get('animations', []):
        channels = []
        duration, start = 0, math.inf
        for channel in animation['channels']:
            target = channel['target']
            if target['path'] == 'weights':
                raise ValueError('animated morph targets are deferred; export skeletal clips')
            if target['path'] not in ('translation', 'rotation', 'scale'):
                raise ValueError('unsupported glTF animation channel')
            sampler = animation['samplers'][channel['sampler']]
            times = [row[0] for row in doc.accessor(sampler['input'])]
            values = doc.accessor(sampler['output'])
            interpolation = sampler.get('interpolation', 'LINEAR')
            if interpolation not in ('STEP', 'LINEAR', 'CUBICSPLINE') or not times or times[0] < 0 or any(b <= a for a, b in zip(times, times[1:])):
                raise ValueError('animation keys must be ordered, nonnegative and use a core interpolation mode')
            if len(values) != len(times) * (3 if interpolation == 'CUBICSPLINE' else 1):
                raise ValueError('animation input/output counts differ')
            duration = max(duration, times[-1])
            start = min(start, times[0])
            channels.append((target['node'], target['path'], times, values, interpolation))
        if not channels:
            raise ValueError('an animation clip must contain channels')
        duration -= start
        # glTF key times are float32; normalize the clip origin before fixed-rate sampling.
        count = math.ceil(duration * fps - 1e-5) + 1
        if count > 65536 or len(frames) + count > 65536:
            raise ValueError('cooked clips exceed the 65536-frame tool limit')
        clip_name = animation.get('name', f'clip{len(clips)}')
        if not clip_name or len(clip_name.encode()) >= 64 or '\0' in clip_name or clip_name in clip_names:
            raise ValueError('clip names must be unique and fit the native 63-byte label')
        clip_names.add(clip_name)
        clips.append((clip_name, len(frames), count, fps, 0))
        for frame in range(count):
            time = start + min(duration, frame / fps)
            overrides = {}
            for node_id, field, times, values, interpolation in channels:
                left = max(0, min(len(times) - 1, bisect.bisect_right(times, time) - 1))
                right = min(left + 1, len(times) - 1)
                dt = times[right] - times[left]
                t = max(0, min(1, (time - times[left]) / dt)) if dt else 0
                if interpolation == 'CUBICSPLINE':
                    a, b = values[left * 3 + 1], values[right * 3 + 1]
                    out, incoming = values[left * 3 + 2], values[right * 3]
                    value = [(2*t**3-3*t*t+1)*x + (t**3-2*t*t+t)*dt*u + (-2*t**3+3*t*t)*y + (t**3-t*t)*dt*v
                             for x, u, y, v in zip(a, out, b, incoming)]
                elif interpolation == 'STEP':
                    value = values[left]
                elif field == 'rotation':
                    value = slerp(values[left], values[right], t)
                else:
                    value = [x + t * (y - x) for x, y in zip(values[left], values[right])]
                overrides.setdefault(node_id, {})[field] = unit(value) if field == 'rotation' else value
            frames.append(worlds(overrides))
    if joints and not frames:
        frames = [rest]
    poses = []
    for frame in frames:
        converted = [mul(mul(basis, frame[joint['node']]), inverse_basis) for joint in joints]
        local = []
        for index, joint in enumerate(joints):
            parent = converted[joint['parent']] if joint['parent'] >= 0 else IDENTITY
            t, r, s = decompose(mul(inverse(parent), converted[index]))
            if poses and dot(r, poses[-1][index][3:7]) < 0:
                r = [-v for v in r]
            local.append([*t, *r, *s])
        poses.append(local)

    outputs, materials = {}, {}

    def material(index):
        if index in materials:
            return materials[index]
        value = source.get('materials', [])[index] if index >= 0 else {}
        label = name + '_material' + str(index if index >= 0 else 'default')
        if workflow == 'metallic-roughness':
            def image(info):
                item = source['textures'][info['index']]
                sampler = source.get('samplers', [])[item['sampler']] if 'sampler' in item else {}
                if (sampler.get('wrapS', 10497) != 10497 or sampler.get('wrapT', 10497) != 10497 or
                        sampler.get('magFilter', 9729) != 9729 or sampler.get('minFilter', 9987) != 9987):
                    raise ValueError('PBR import requires repeat/linear mipmapped samplers; bake other sampling first')
                return doc.image(item['source'])
            outputs.update(pbr_material(value, label, image))
            materials[index] = label
            return label
        base = value.get('pbrMetallicRoughness', {}).get('baseColorTexture')
        if base:
            if base.get('texCoord', 0) != 0 or base.get('extensions'):
                raise ValueError('material texture transforms/additional UV sets need baking before this IQM import')
            raw = doc.image(source['textures'][base['index']]['source'])
        else:
            stream = io.BytesIO()
            Image.new('RGBA', (1, 1), (255, 255, 255, 255)).save(stream, format='PNG')
            raw = stream.getvalue()
        image_name = label + '.ktx2'
        outputs[label + '.asmat'] = material_bytes(value, image_name)
        outputs[image_name] = material_texture(raw, value)
        materials[index] = label
        return label

    vertices, triangles, meshes = [], [], []
    for node_id in mesh_nodes:
        node = nodes[node_id]
        mesh = source['meshes'][node['mesh']]
        transform = basis if joints else mul(basis, rest[node_id])
        columns = [[transform[r * 4 + c] for r in range(3)] for c in range(3)]
        mirrored = dot(cross(columns[0], columns[1]), columns[2]) < 0
        for primitive in mesh['primitives']:
            attributes = {key: doc.accessor(value) for key, value in primitive['attributes'].items()}
            positions = attributes['POSITION']
            if not positions or any(len(value) != len(positions) for value in attributes.values()):
                raise ValueError('mesh attribute counts must agree and contain vertices')
            indices = [row[0] for row in doc.accessor(primitive['indices'])] if 'indices' in primitive else list(range(len(positions)))
            if any(not isinstance(index, int) or not 0 <= index < len(positions) for index in indices):
                raise ValueError('mesh index outside its vertex attributes')
            mode = primitive.get('mode', 4)
            if mode == 4 and len(indices) % 3 == 0:
                faces = [indices[i:i + 3] for i in range(0, len(indices), 3)]
            elif mode == 5:
                faces = [[indices[i + (i % 2)], indices[i + 1 - (i % 2)], indices[i + 2]] for i in range(len(indices) - 2)]
            elif mode == 6:
                faces = [[indices[0], indices[i], indices[i + 1]] for i in range(1, len(indices) - 1)]
            else:
                raise ValueError('IQM import requires triangles, triangle strips or triangle fans')
            shader = material(primitive.get('material', -1))
            mesh_name = node.get('name', mesh.get('name', 'mesh'))
            local_vertices, local_faces, mapping = [], [], {}

            def flush():
                if not local_faces:
                    return
                first = len(vertices)
                meshes.append((mesh_name, shader, first, len(local_vertices), len(triangles), len(local_faces)))
                vertices.extend(local_vertices)
                triangles.extend([[first + index for index in face] for face in local_faces])
                local_vertices.clear()
                local_faces.clear()
                mapping.clear()

            for face_number, face in enumerate(faces):
                if len(local_vertices) + 3 >= 1000 or len(local_faces) + 1 >= 2000:
                    flush()
                flat = None
                if 'NORMAL' not in attributes:
                    a, b, c = [positions[i] for i in face]
                    flat = unit(cross([y-x for x,y in zip(a,b)], [y-x for x,y in zip(a,c)]))
                converted_face = []
                for source_index in face:
                    key = (source_index, face_number if flat is not None else -1)
                    if key not in mapping:
                        n = normal(transform, flat if flat is not None else attributes['NORMAL'][source_index])
                        uv = attributes.get('TEXCOORD_0', [[0, 0]] * len(positions))[source_index]
                        color = list(attributes.get('COLOR_0', [[1, 1, 1, 1]] * len(positions))[source_index])
                        color = [max(0, min(255, round(v * 255))) for v in (color + [1])[:4]]
                        if 'skin' in node:
                            j = attributes['JOINTS_0'][source_index]
                            w = attributes['WEIGHTS_0'][source_index]
                            if len(j) != 4 or len(w) != 4 or sum(w) <= 0:
                                raise ValueError('skinned vertices require four joint/weight components')
                            if 'WEIGHTS_1' in attributes and any(attributes['WEIGHTS_1'][source_index]):
                                raise ValueError('IQM supports four influences; export with four bone influences')
                            remap = skin_maps[node['skin']]
                            if any(not isinstance(index, int) or not 0 <= index < len(remap) for index in j):
                                raise ValueError('joint index outside the mesh skin')
                            j, w = [remap[index] for index in j], [value / sum(w) for value in w]
                        else:
                            j, w = [rigid.get(node_id, 0), 0, 0, 0], [1, 0, 0, 0]
                        tangent = attributes.get('TANGENT')
                        if tangent:
                            t = unit(point(transform, tangent[source_index][:3], 0)) + [tangent[source_index][3] * (-1 if mirrored else 1)]
                        else:
                            t = None
                        mapping[key] = len(local_vertices)
                        local_vertices.append((point(transform, positions[source_index]), n, uv, j, w, t, color))
                    converted_face.append(mapping[key])
                # glTF faces are counter-clockwise; the engine's IQM path uses clockwise.
                local_faces.append(converted_face if mirrored else [converted_face[0], converted_face[2], converted_face[1]])
            flush()
    outputs[name + '.iqm'] = pack_iqm(vertices, triangles, meshes, joints, clips, poses, doc, options)
    return outputs


def pack_iqm(vertices, triangles, meshes, joints, clips, poses, doc, options):
    text, names = bytearray(b'\0'), {'': 0}

    def name(value):
        if value not in names:
            names[value] = len(text)
            text.extend(value.encode() + b'\0')
        return names[value]

    def float32(value):
        return struct.unpack('<f', struct.pack('<f', value))[0]

    mesh_rows = [(name(m[0]), name(m[1]), *m[2:]) for m in meshes]
    joint_rows = []
    for joint in joints:
        t, r, s = joint['local']
        joint_rows.append(struct.pack('<Ii10f', name(joint['name']), joint['parent'], *t, *r, *s))
    clip_rows = [struct.pack('<IIIfI', name(n), first, count, fps, flags) for n, first, count, fps, flags in clips]
    extension_name = name('aftershock.cook')
    data = bytearray(124)

    def add(value):
        data.extend(bytes(-len(data) % 4))
        offset = len(data)
        data.extend(value)
        return offset

    h = [0] * 27
    h[0], h[3], h[4] = 2, len(text), add(text)
    h[5], h[6] = len(meshes), add(b''.join(struct.pack('<6I', *row) for row in mesh_rows))
    arrays = []
    for kind, element, fmt, components, field in [(0, 7, 'f', 3, 0), (1, 7, 'f', 2, 2),
                                                  (2, 7, 'f', 3, 1),
                                                  *([(3, 7, 'f', 4, 5)] if all(v[5] is not None for v in vertices) else []),
                                                  (6, 1, 'B', 4, 6),
                                                  *([(4, 1, 'B', 4, 3), (5, 7, 'f', 4, 4)] if joints else [])]:
        values = [value for vertex in vertices for value in vertex[field]]
        offset = add(struct.pack('<' + fmt * len(values), *values))
        arrays.append(struct.pack('<5I', kind, 0, element, components, offset))
    h[7], h[8], h[9] = len(arrays), len(vertices), add(b''.join(arrays))
    h[10], h[11] = len(triangles), add(b''.join(struct.pack('<3I', *face) for face in triangles))
    h[13], h[14] = len(joints), add(b''.join(joint_rows))
    pose_rows, offsets, scales, masks = [], [], [], []
    for index, joint in enumerate(joints):
        values = [pose[index] for pose in poses]
        lo = [min(row[c] for row in values) for c in range(10)]
        hi = [max(row[c] for row in values) for c in range(10)]
        step = [(b - a) / 65535 if b != a else 0 for a, b in zip(lo, hi)]
        mask = sum(1 << c for c, value in enumerate(step) if value)
        offsets.append(lo)
        scales.append(step)
        masks.append(mask)
        pose_rows.append(struct.pack('<iI20f', joint['parent'], mask, *lo, *step))
    h[15], h[16] = len(joints), add(b''.join(pose_rows))
    h[17], h[18], h[19] = len(clips), add(b''.join(clip_rows)), len(poses)
    h[20] = sum(mask.bit_count() for mask in masks)
    channels, decoded_frames = [], []
    for frame in poses:
        matrices = []
        for index, values in enumerate(frame):
            decoded = list(map(float32, offsets[index]))
            for c in range(10):
                if masks[index] & (1 << c):
                    encoded = max(0, min(65535, round((values[c] - offsets[index][c]) / scales[index][c])))
                    channels.append(encoded)
                    decoded[c] = float32(decoded[c] + float32(encoded * float32(scales[index][c])))
            matrix = trs(decoded[:3], decoded[3:7], decoded[7:])
            parent = joints[index]['parent']
            matrices.append(mul(matrices[parent], matrix) if parent >= 0 else matrix)
        decoded_frames.append(matrices)
    h[21] = add(struct.pack('<' + 'H' * len(channels), *channels))
    # Bounds describe the stored, quantized poses and float vertices, not the
    # higher precision source samples that those records approximate.
    bind_worlds = []
    for joint in joints:
        translation, rotation, scale = [list(map(float32, part)) for part in joint['local']]
        matrix = trs(translation, rotation, scale)
        bind_worlds.append(mul(bind_worlds[joint['parent']], matrix) if joint['parent'] >= 0 else matrix)
    inverse_binds = [inverse(matrix) for matrix in bind_worlds]
    bounds = []
    for matrices in decoded_frames:
        skin = [mul(matrix, bind) for matrix, bind in zip(matrices, inverse_binds)]
        positions = []
        for vertex in vertices:
            position, weights = list(map(float32, vertex[0])), list(map(float32, vertex[4]))
            positions.append([sum(point(skin[j], position)[c] * w for j, w in zip(vertex[3], weights)) for c in range(3)])
        # Cover float matrix/skin accumulation and outward rounding of bounds.
        padding = max(0.001, max(abs(c) for v in positions for c in v) * 1e-5 * (len(joints) + 1))
        lo = [min(v[c] for v in positions) - padding for c in range(3)]
        hi = [max(v[c] for v in positions) + padding for c in range(3)]
        bounds.append(struct.pack('<8f', *lo, *hi, max(math.hypot(*v[:2]) for v in positions) + padding, max(math.sqrt(dot(v, v)) for v in positions) + padding))
    h[22] = add(b''.join(bounds)) if bounds else 0
    source_hash = hashlib.sha256(json.dumps(doc.data, sort_keys=True).encode() + b''.join(doc.buffers) + json.dumps(options, sort_keys=True).encode()).digest()
    metadata = add(struct.pack('<I32s32s', 1, source_hash, bytes(32)))
    h[25], h[26] = 1, add(struct.pack('<4I', extension_name, 68, metadata, 0))
    h[1] = len(data)
    if len(data) > 16 << 20:
        raise ValueError('cooked IQM exceeds the existing 16 MiB engine limit')
    struct.pack_into('<16s27I', data, 0, b'INTERQUAKEMODEL\0', *h)
    data[metadata + 36:metadata + 68] = hashlib.sha256(data).digest()
    return bytes(data)
