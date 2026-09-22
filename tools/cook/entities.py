"""Resolve prefab inheritance into bounded component records for native entities."""
import json
import struct
from model import wrapped


def text(value,capacity=64):
    encoded=value.encode('ascii')
    if len(encoded)>=capacity or b'\0' in encoded:
        raise ValueError('entity field exceeds its record capacity or contains NUL')
    return encoded


def cook(source,name,read):
    from tools.agent.formats import entities_schema
    document=json.loads(read(source))
    definitions={row['id']:row for row in document['definitions']}
    if len(definitions)!=len(document['definitions']):
        raise ValueError('entity definition ids must be unique')
    schemas=entities_schema()['properties']['definitions']['items']['properties']['components']['properties']
    resolved={}
    def resolve(identity,parents=()):
        if identity in parents or len(parents)>=16:
            raise ValueError('entity prefab cycle or inheritance exceeds 16 levels')
        if identity not in definitions:
            raise ValueError('missing entity prefab '+identity)
        if identity in resolved:
            return resolved[identity]
        row=definitions[identity]
        base=resolve(row['extends'],parents+(identity,)) if 'extends' in row else dict(native='',components={})
        components={key:dict(value) for key,value in base['components'].items()}
        for key,value in row['components'].items():
            components.setdefault(key,{}).update(value)
        result=dict(native=row.get('native',base['native']),components=components)
        if not result['native']:
            raise ValueError('entity definition requires an inherited or explicit native behavior')
        resolved[identity]=result
        return result
    def decimal(value):
        if isinstance(value,str):
            return value
        if isinstance(value,list):
            return ' '.join(decimal(item) for item in value)
        return str(value) if isinstance(value,int) else format(value,'.9f').rstrip('0').rstrip('.')
    records=bytearray()
    properties=bytearray()
    field_count=0
    for identity in definitions:
        row=resolve(identity)
        first=field_count
        mask=0
        for index,(component,schema) in enumerate(schemas.items()):
            if component not in row['components']:
                continue
            mask|=1<<index
            for key,value in row['components'][component].items():
                field=schema['properties'][key].get('x-spawn-field')
                if field:
                    properties.extend(struct.pack('<16s32s128s',text(component,16),text(field,32),text(decimal(value),128)))
                    field_count+=1
        if field_count-first>32 or field_count>2048:
            raise ValueError('entity definitions exceed 32 fields per prefab or 2048 total')
        replication=row['components'].get('replication',{})
        records.extend(struct.pack('<64s64sIIIIf',text(identity),text(row['native']),first,field_count-first,mask,
                                   replication.get('priority',0),replication.get('radius',0)))
    payload=struct.pack('<32sII',text(document['name'],32),len(definitions),field_count)+records+properties
    return {name+'.asent':wrapped(b'ASENT\0\0\0',payload)}
