#!/usr/bin/env python3
"""Validate and cook the frozen, original reference effect set without regenerating art."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from cook import cook
from run import ROOT,SCRATCH

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-effects-reference')
args=parser.parse_args()
fixture=ROOT/'tests/assets/effects'
provenance=json.loads((fixture/'provenance.json').read_text())
assert provenance['license']=='CC0-1.0'
for name,expected in provenance['files'].items():
    assert hashlib.sha256((fixture/name).read_bytes()).hexdigest()==expected,name
# The original shell must have outward face normals before entering the cooker.
document=json.loads((fixture/'shell.gltf').read_text())
blob=(fixture/'shell.bin').read_bytes()
primitive=document['meshes'][0]['primitives'][0]
def values(index,fmt,count):
    accessor=document['accessors'][index]
    view=document['bufferViews'][accessor['bufferView']]
    return list(struct.iter_unpack('<'+fmt*count,blob[view['byteOffset']:view['byteOffset']+view['byteLength']]))
positions=values(primitive['attributes']['POSITION'],'f',3)
normals=values(primitive['attributes']['NORMAL'],'f',3)
indices=[row[0] for row in values(primitive['indices'],'H',1)]
for start in range(0,len(indices),3):
    a,b,c=[positions[index] for index in indices[start:start+3]]
    u=[y-x for x,y in zip(a,b)];v=[y-x for x,y in zip(a,c)]
    cross=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
    normal=normals[indices[start]]
    assert sum(x*y for x,y in zip(cross,normal))>0
    center=[sum(point[axis] for point in (a,b,c))/3-(3 if axis==0 else 0) for axis in range(3)]
    assert sum(x*y for x,y in zip(center,normal))>0
names=('muzzle','impact_metal','impact_stone','smoke','sparks','dust','shell','explosion','tracer')
definitions={name:json.loads((fixture/(name+'.json')).read_text()) for name in names}
assert definitions['shell']['emitters'][0]['kind']=='mesh'
assert definitions['tracer']['emitters'][0]['kind']=='trail'
assert definitions['smoke']['emitters'][0]['soft'] and definitions['smoke']['emitters'][0]['lit']
assert any(e.get('light',{}).get('intensity',0)>0 for e in definitions['muzzle']['emitters'])
assert any(e.get('collision') for e in definitions['sparks']['emitters'])
assert all(sum(e['capacity'] for e in definition['emitters'])<=256 for definition in definitions.values())
cook(fixture/'assets.json',args.output)
for name in names:
    assert (args.output/f'effects/reference/{name}.asfx').is_file()
assert (args.output/'models/reference_shell.iqm').is_file()
assert cook(fixture/'assets.json',args.output)['built']==[]
print('PASS: original reference effects, fixed provenance, required emitter paths and incremental cooking')
