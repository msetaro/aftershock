"""Cook validated presentation emitters into bounded native records."""
import json
import struct

from model import wrapped
from weapon import text


def cook(source,name,read):
    definition=json.loads(read(source))
    emitters=definition['emitters']
    if len({row['name'] for row in emitters})!=len(emitters):
        raise ValueError('duplicate effect emitter name')
    data=bytearray(struct.pack('<32sI',text(definition['name'],32),len(emitters)))
    for row in emitters:
        flip=row.get('flipbook',dict(columns=1,rows=1,fps=1))
        flags=sum(bit for field,bit in [('collision',1),('soft',2),('lit',4)] if row.get(field,False))
        data.extend(struct.pack('<32s64s64s7I29f',text(row['name'],32),text(row['material']),
            text(row['model']) if row.get('model') else bytes(64),
            {'sprite':0,'mesh':1,'trail':2}[row['kind']],row['capacity'],row['burst'],row['lifetime_ms'],flags,
            flip['columns'],flip['rows'],row['rate'],row['size'],*row.get('velocity',[0,0,0]),
            *row.get('gravity',[0,0,0]),row.get('drag',0),*row.get('color',[1,1,1,1]),flip['fps'],
            *row.get('velocity_spread',[0,0,0]),*row.get('origin_spread',[0,0,0]),row.get('end_size',row['size']),
            row.get('rotation',0),row.get('rotation_spread',0),row.get('angular_velocity',0),
            row.get('light',{}).get('radius',0),row.get('light',{}).get('intensity',0),*row.get('light',{}).get('color',[1,1,1])))
    return {name+'.asfx':wrapped(b'ASEFFECT',data,version=2)}


def cook_decal(source,name,read):
    definition=json.loads(read(source))
    if definition['fade_ms']>definition['lifetime_ms']:
        raise ValueError('fade_ms must not exceed lifetime_ms')
    data=struct.pack('<32s64s64sII8f',text(definition['name'],32),text(definition['color_map']),text(definition['normal_map']),
                     definition['lifetime_ms'],definition['fade_ms'],*[size*.5 for size in definition['size']],
                     *definition['color'],definition['normal_strength'])
    return {name+'.asdc':wrapped(b'ASDECAL\0',data)}
