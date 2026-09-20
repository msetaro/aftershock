"""Cook bounded weapon definitions; runtime consumes flat POD, not JSON."""
import json
import math
import re
import struct
import model


def number(value, lo, hi):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or not lo <= value <= hi:
        raise ValueError(f'weapon value must be finite and in {lo}..{hi}')
    return value


def integer(value, lo, hi):
    if type(value) is not int:
        raise ValueError('weapon counter/time must be an integer')
    return number(value, lo, hi)


def text(value, length=64):
    if not isinstance(value, str) or not re.fullmatch(r'[a-z0-9_./-]+', value) or value.startswith('/') or '..' in value:
        raise ValueError('weapon names and paths must be relative lowercase identifiers')
    data = value.encode('ascii')
    if len(data) >= length:
        raise ValueError('weapon name/path exceeds its fixed field')
    return data.ljust(length, b'\0')


def cook(source, name, read):
    d = json.loads(read(source))
    if d.get('version') != 1:
        raise ValueError('weapon definition version must be 1')
    recoil, reload = d['recoil'], d['reload']
    materials, attachments, sounds = d['materials'], d['attachments'], d['sounds']
    for rows, minimum, maximum in [(recoil, 1, 32), (reload, 1, 8), (materials, 1, 8), (attachments, 0, 8), (sounds, 0, 8)]:
        integer(len(rows), minimum, maximum)
    if d['falloff_end'] <= d['falloff_start'] or d['range'] < d['falloff_end'] or d['minimum_damage'] > d['damage']:
        raise ValueError('weapon falloff and damage endpoints must be ordered')
    data = bytearray(b''.join(text(d[k]) for k in ('name', 'model', 'animation')))
    data.extend(struct.pack('<4I', {'auto': 0, 'semi': 1, 'burst': 2}[d['fire_mode']],
        integer(d['interval_ms'], 20, 60000), integer(d['burst_count'], 1, 32), {'hitscan': 0, 'projectile': 1}[d['ballistics']]))
    data.extend(struct.pack('<8f', *[number(d[k], 0, maximum) for k, maximum in [
        ('damage', 10000), ('minimum_damage', 10000), ('falloff_start', 65536), ('falloff_end', 65536),
        ('range', 65536), ('spread_degrees', 90), ('ads_spread_scale', 4), ('view_kick_scale', 4)]]))
    data.extend(struct.pack('<I3f8I', integer(d['ads_ms'], 20, 10000), number(d['ads_fov'], 1, 179),
        number(d['sway'], 0, 10), number(d['bob'], 0, 10), integer(d['magazine'], 1, 1000), integer(d['reserve'], 0, 65535),
        len(recoil), len(reload), len(materials), len(attachments), len(sounds), integer(d['switch_ms'], 20, 10000)))
    projectile, melee = d['projectile'], d['melee']
    data.extend(struct.pack('<4fI', number(projectile['speed'], 1, 8192), number(projectile['gravity'], 0, 4096),
        number(projectile['bounce'], 0, 1), number(projectile['radius'], 0, 4096), integer(projectile['fuse_ms'], 20, 60000)))
    data.extend(struct.pack('<2fI', number(melee['range'], 0, 256), number(melee['damage'], 0, 10000), integer(melee['interval_ms'], 20, 60000)))
    for row in recoil:
        if len(row) != 2:
            raise ValueError('recoil rows require pitch/yaw')
        data.extend(struct.pack('<2f', *[number(v, -90, 90) for v in row]))
    data.extend(bytes((32 - len(recoil)) * 8))
    previous = -1
    for row in reload:
        time = integer(row['time_ms'], 0, 10000)
        if time <= previous or type(row['cancel']) is not bool:
            raise ValueError('reload stages need ordered times and boolean cancel points')
        previous = time
        data.extend(struct.pack('<3I64s', time, {'eject': 0, 'insert': 1, 'chamber': 2, 'finish': 3}[row['action']], int(row['cancel']), text(row['event'])))
    if [row['action'] for row in reload] != ['eject', 'insert', 'chamber', 'finish']:
        raise ValueError('magazine reload requires eject/insert/chamber/finish stages')
    data.extend(bytes((8 - len(reload)) * 76))
    if materials[0]['surface_flags'] != 0:
        raise ValueError('first material must be the default surface')
    for row in materials:
        data.extend(struct.pack('<32sI2f64s', text(row['name'], 32), integer(row['surface_flags'], 0, 0xffffffff),
            number(row['depth'], 0, 256), number(row['damage_scale'], 0, 1), text(row['effect'])))
    data.extend(bytes((8 - len(materials)) * 108))
    for row in attachments:
        data.extend(struct.pack('<32s32s64s3f', text(row['name'], 32), text(row['socket'], 32), text(row['model']),
            number(row['spread_scale'], 0, 4), number(row['recoil_scale'], 0, 4), number(row['ads_fov'], 1, 179)))
    data.extend(bytes((8 - len(attachments)) * 140))
    for event, path in sorted(sounds.items()):
        data.extend(text(event, 32) + text(path))
    data.extend(bytes((8 - len(sounds)) * 96))
    assert len(data) == 3936
    return {name + '.asweapon': model.wrapped(b'ASWEAP\0\0', data)}
