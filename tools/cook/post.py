"""Cook presentation-only post profiles into fixed native settings."""
import json
import struct
from model import wrapped
from weapon import text


def cook(source,name,read):
    definition=json.loads(read(source))
    fields=(('exposure_ev',0),('sharpen',0),('vignette',0),('grain',0),('lut_strength',1),
            ('focus_distance',256),('focus_range',128),('dof_radius',0),('motion_blur',0))
    payload=struct.pack('<32s64s9f',text(definition['name'],32),text(definition['lut']) if definition.get('lut') else bytes(64),
                        *[definition.get(field,default) for field,default in fields])
    return {name+'.aspost':wrapped(b'ASPOST\0\0',payload)}
