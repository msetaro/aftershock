"""Compile animation graphs around the existing IQM sampler/compression output."""
import hashlib
import json
import math
import re
import struct

import model


def name(value):
    if not isinstance(value, str) or not value or '\0' in value or len(value.encode()) > 63:
        raise ValueError('animation names must be nonempty and at most 63 UTF-8 bytes')
    return value.encode()


def number(value):
    value = float(value)
    if not math.isfinite(value) or abs(value) > 3.4028234e38:
        raise ValueError('animation values must be finite float32 values')
    return value


def integer(value, minimum, maximum):
    if isinstance(value, bool) or not isinstance(value, int) or not minimum <= value <= maximum:
        raise ValueError('animation integer is outside its supported range')
    return value


def named(rows, limit):
    result = {}
    if len(rows) > limit:
        raise ValueError('animation table exceeds its runtime capacity')
    for row in rows:
        key = row['name']
        name(key)
        if key in result:
            raise ValueError('duplicate animation name: ' + key)
        result[key] = len(result)
    return result


def cook(path, asset_name, options, read, assets):
    graph = json.loads(read(path))
    if graph.get('version') != 1:
        raise ValueError('animation graph version must be 1')
    model_path = graph['model_asset']
    if not re.fullmatch(r'[a-z0-9_/-]+\.iqm', model_path) or model_path.startswith('/') or '..' in model_path:
        raise ValueError('animation model must be a relative lowercase IQM qpath')
    name(model_path)
    matching = [a for a in assets if a['kind'] == 'model' and a['name'] + '.iqm' == model_path]
    if len(matching) != 1:
        raise ValueError('animation model must have one model asset in this project')
    model_options = matching[0]
    model_source = (path.parent / graph['model_source']).resolve()
    # Project model sources are resolved by the caller before reaching this function.
    if model_source != model_options['_source']:
        raise ValueError('animation and model recipes must refer to the same glTF source')
    recipe = {k: v for k, v in model_options.items() if k != '_source'}
    for key in ('scale', 'fps'):
        if key in options and options[key] != recipe.get(key, 32 if key == 'scale' else 30):
            raise ValueError('animation and model sampling settings must match')
    # ponytail: recook the model on graph edits; share offline results if this becomes a measured bottleneck.
    iqm = model.cook(model_source, model_path[:-4], recipe, read)[model_path]
    h = struct.unpack_from('<27I', iqm, 16)
    if not 1 <= h[13] <= 128 or h[15] != h[13] or not h[17]:
        raise ValueError('animation requires a skeleton with 1..128 joints and named clips')
    text = iqm[h[4]:h[4] + h[3]]

    def label(offset):
        return text[offset:text.index(0, offset)].decode()

    sections = [[] for _ in range(12)]
    joints, parents, clips, channel = {}, [], {}, 0
    for i in range(h[13]):
        joint = struct.unpack_from('<Ii10f', iqm, h[14] + i * 48)
        pose = struct.unpack_from('<iI20f', iqm, h[16] + i * 88)
        key = label(joint[0])
        if key in joints:
            raise ValueError('animation joint names must be unique')
        joints[key] = i
        parents.append(joint[1])
        sections[0].append(struct.pack('<64siI10f', name(key), joint[1], channel, *joint[2:]))
        sections[1].append(struct.pack('<I20f', *pose[1:]))
        channel += pose[1].bit_count()
    sections[2] = [iqm[h[21] + i * 2:h[21] + i * 2 + 2] for i in range(h[19] * h[20])]
    durations = []
    for i in range(h[17]):
        key, first, count, fps, flags = struct.unpack_from('<3IfI', iqm, h[18] + i * 20)
        key = label(key)
        clips[key] = i
        duration = max(1, round((count - 1) * 1000 / fps))
        durations.append(duration)
        sections[3].append(struct.pack('<64s4I', name(key), first, count, duration, flags))
    parameters = graph.get('parameters', [])
    parameter_ids = named(parameters, 16)
    for parameter in parameters:
        lo, hi = number(parameter.get('min', -1e30)), number(parameter.get('max', 1e30))
        default = number(parameter.get('default', 0))
        if not lo <= default <= hi:
            raise ValueError('animation parameter default is outside its range')
        sections[4].append(struct.pack('<64s3f', name(parameter['name']), default, lo, hi))
    masks = graph.get('masks', [])
    mask_ids = named(masks, 16)
    for mask in masks:
        weights = [0.0] * 128
        root = joints[mask['root']] if 'root' in mask else -1
        for joint, index in joints.items():
            ancestor = index
            while ancestor >= 0 and ancestor != root:
                ancestor = parents[ancestor]
            weights[index] = 1.0 if root >= 0 and ancestor == root else 0.0
        for joint, value in mask.get('weights', {}).items():
            value = number(value)
            if not 0 <= value <= 1:
                raise ValueError('bone mask weights must be in [0,1]')
            weights[joints[joint]] = value
        sections[10].append(struct.pack('<64s128f', name(mask['name']), *weights))
    nodes = graph.get('nodes', [])
    node_ids = named(nodes, 64)
    for index, node in enumerate(nodes):
        kinds = [key for key in ('clip', 'blend', 'additive') if key in node]
        if len(kinds) != 1:
            raise ValueError('each animation node requires exactly one clip/blend/additive operation')
        kind = ('clip', 'blend', 'additive').index(kinds[0])
        a, b, reference = 0, 0, 0
        if kind == 0:
            a = clips[node['clip']]
        else:
            children = node[kinds[0]]
            if len(children) != 2:
                raise ValueError('blend and additive nodes require two children')
            a, b = [node_ids[child] for child in children]
            reference = node_ids[node['reference']] if kind == 2 else 0
            if max(a, b, reference) >= index:
                raise ValueError('animation nodes must follow their child/reference nodes')
        weight = number(node.get('weight', 1))
        if not 0 <= weight <= 1:
            raise ValueError('animation node weights must be in [0,1]')
        parameter = parameter_ids[node['parameter']] if 'parameter' in node else 0xffffffff
        mask = mask_ids[node['mask']] if 'mask' in node else 0xffffffff
        sections[9].append(struct.pack('<64s7If', name(node['name']), kind, a, b, reference,
                                       parameter, mask, int(bool(node.get('loop', False))), weight))
    states = graph['states']
    state_ids = named(states, 64)
    if not states:
        raise ValueError('animation graph requires a state')
    for state in states:
        clip = clips[state['clip']]
        first_event = len(sections[8])
        last_time = -1
        for event in state.get('events', []):
            time = integer(event['time_ms'], 0, durations[clip])
            if time < last_time or (state.get('loop', False) and time == durations[clip]):
                raise ValueError('events must be ordered; loop endpoint events belong at time zero')
            last_time = time
            sections[8].append(struct.pack('<64sIi', name(event['name']), time, joints[event['bone']] if event.get('bone') else -1))
        speed = number(state.get('speed', 1))
        if not 1 / 65536 <= speed <= 16:
            raise ValueError('animation state speed must be in [1/65536,16]')
        if 'node' in state:
            node = node_ids[state['node']]
        else:
            node = len(sections[9])
            sections[9].append(struct.pack('<64s7If', name(state['name']), 0, clip, 0, 0, 0xffffffff,
                                           0xffffffff, int(bool(state.get('loop', False))), 1))
        sections[5].append(struct.pack('<64s6I', name(state['name']), clip, int(bool(state.get('loop', False))),
                                       round(speed * 65536), first_event, len(sections[8]) - first_event, node))
    operations = {'==': 0, '!=': 1, '<': 2, '<=': 3, '>': 4, '>=': 5}
    for transition in graph.get('transitions', []):
        first = len(sections[7])
        for condition in transition.get('conditions', []):
            sections[7].append(struct.pack('<IIf', parameter_ids[condition['parameter']], operations[condition['op']], number(condition['value'])))
        sections[6].append(struct.pack('<6I', 0xffffffff if transition['from'] == '*' else state_ids[transition['from']],
                                       state_ids[transition['to']], integer(transition.get('blend_ms', 0), 0, 60000),
                                       first, len(sections[7]) - first, int(bool(transition.get('on_end', False)))))
    if len(sections[9]) > 64:
        raise ValueError('animation graph exceeds 64 nodes including implicit state clips')
    boxes = graph.get('hit_boxes', [])
    named(boxes, 32)
    for box in boxes:
        offset = [number(v) for v in box.get('offset', [0, 0, 0])]
        extent = [number(v) for v in box['extent']]
        if len(offset) != 3 or len(extent) != 3 or any(v <= 0 for v in extent):
            raise ValueError('hit boxes require three offsets and positive half extents')
        sections[11].append(struct.pack('<64sI6f', name(box['name']), joints[box['bone']], *offset, *extent))
    payload = bytearray(204)
    struct.pack_into('<64s32s3I', payload, 0, name(model_path), hashlib.sha256(iqm).digest(), h[19], h[20], state_ids[graph['initial_state']])
    for i, rows in enumerate(sections):
        payload.extend(bytes(-len(payload) % 4))
        struct.pack_into('<II', payload, 108 + i * 8, len(rows), len(payload))
        payload.extend(b''.join(rows))
    if len(payload) > 16 << 20:
        raise ValueError('cooked animation exceeds 16 MiB')
    return {asset_name + '.asanim': model.wrapped(b'ASANIM\0\0', payload)}
