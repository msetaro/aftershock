"""Bounded hierarchical state-machine tables; game actions remain native."""
import json
import struct
from model import wrapped
from entities import text

ACTIONS = ('idle', 'patrol', 'investigate', 'cover', 'attack')
FIELDS = ('visible', 'heard', 'health', 'covered', 'time_ms', 'distance')
OPS = ('eq', 'ne', 'lt', 'le', 'gt', 'ge')


def cook(path, name, read):
    document = json.loads(read(path))
    states = document['states']
    indices = {row['name']: i for i, row in enumerate(states)}
    if len(indices) != len(states) or document['initial'] not in indices:
        raise ValueError('behavior state names must be unique and initial state must exist')
    parents = []
    for row in states:
        if 'parent' in row and row['parent'] not in indices:
            raise ValueError('behavior parent must name an existing state')
        parents.append(indices[row['parent']] if 'parent' in row else 0xffffffff)
    for i in range(len(states)):
        seen = set()
        while i != 0xffffffff:
            if i in seen or len(seen) == 8:
                raise ValueError('behavior hierarchy cycles or exceeds eight levels')
            seen.add(i)
            i = parents[i]
    initial = indices[document['initial']]
    if initial in parents:
        raise ValueError('behavior initial state must be a leaf')
    records, transitions = bytearray(), bytearray()
    count = 0
    for i, row in enumerate(states):
        rules = row['transitions']
        records.extend(struct.pack('<32s4I', text(row['name'], 32), parents[i], ACTIONS.index(row['action']), count, len(rules)))
        for rule in rules:
            target = indices.get(rule['to'])
            if target is None or target in parents:
                raise ValueError('behavior transition target must name a leaf state')
            field, value, op = rule['field'], rule['value'], rule['op']
            if field in ('visible', 'heard', 'covered') and (value not in (0, 1) or op not in ('eq', 'ne')):
                raise ValueError('boolean behavior fields require equality to zero or one')
            maximum = dict(health=1, time_ms=3600000, distance=1048576).get(field, 1)
            if value < 0 or value > maximum:
                raise ValueError('behavior field comparison is outside its range')
            transitions.extend(struct.pack('<IIIfI', target, FIELDS.index(field), OPS.index(op), value, rule['min_ms']))
            count += 1
    if count > 128:
        raise ValueError('behavior exceeds 128 transitions')
    header = struct.pack('<32s3I', text(document['name'], 32), len(states), count, initial)
    return {name+'.asai': wrapped(b'ASAI\0\0\0\0', header+records+transitions)}
