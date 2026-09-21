#!/usr/bin/env python3
"""Author the original reference set once into an empty directory; CI never runs this."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct

from PIL import Image

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
out=args.output.resolve()
out.mkdir(parents=True,exist_ok=True)
if any(path.name!='export.py' for path in out.iterdir()):
    raise SystemExit('refusing to overwrite an authored reference set')
(out/'export.py').write_bytes(Path(__file__).read_bytes())

def write(name,value):
    (out/name).write_text(json.dumps(value,indent=2)+'\n')

rng=random.Random(161)
puffs=[(rng.uniform(-.45,.45),rng.uniform(-.45,.45),rng.uniform(.15,.32),rng.uniform(.7,1.1)) for _ in range(14)]
for name in ('smoke','fire','flash','spark'):
    frames=8 if name in ('smoke','fire') else 1
    side=128 if name!='spark' else 64
    columns,rows=(4,2) if frames==8 else (1,1)
    image=Image.new('RGBA',(side*columns,side*rows))
    for frame in range(frames):
        pixels=[]
        phase=frame/8
        for y in range(side):
            for x in range(side):
                u,v=(x+.5)/side*2-1,(y+.5)/side*2-1
                radius=math.hypot(u,v)
                edge=max(0,1-radius)**.5
                if name in ('smoke','fire'):
                    density=sum(weight*math.exp(-((u-px*(1+.35*phase))**2+(v-py+.12*phase)**2)/(2*(size*(.8+.6*phase))**2))
                                for px,py,size,weight in puffs)
                    noise=.82+.18*math.sin(21*u+3*math.sin(13*v)+phase*3)
                    alpha=min(1,density*.28)*edge*noise
                    if name=='smoke':
                        shade=max(0,min(255,round(155+85*edge-25*density)))
                        color=(shade,shade,shade)
                    else:
                        heat=max(0,min(1,density*.24*(1-.45*phase)))
                        color=(255,round(50+205*heat),round(8+170*heat**3))
                        alpha*=1-.6*phase
                elif name=='flash':
                    angle=math.atan2(v,u)
                    rays=(.5+.5*math.cos(angle*6))**8
                    alpha=min(1,math.exp(-radius*radius*24)+rays*math.exp(-radius*5))*edge
                    color=(255,round(160+95*math.exp(-radius*8)),round(60+195*math.exp(-radius*10)))
                else:
                    alpha=math.exp(-(u*u*8+v*v*48))*edge
                    color=(255,255,255)
                pixels.append((*color,max(0,min(255,round(alpha*255)))))
        tile=Image.new('RGBA',(side,side))
        tile.putdata(pixels)
        image.paste(tile,((frame%columns)*side,(frame//columns)*side))
    image.save(out/(name+'.png'),compress_level=9)

# Original short brass cylinder, flat-normal faces, glTF metres mapped as model units.
positions,normals,uvs,indices=[],[],[],[]
def face(points,normal):
    start=len(positions)
    positions.extend(points)
    normals.extend([normal]*len(points))
    uvs.extend([(0,0),(1,0),(1,1),(0,1)][:len(points)])
    indices.extend([start,start+2,start+1])
    if len(points)==4:
        indices.extend([start,start+3,start+2])
for i in range(12):
    a,b=i*math.tau/12,(i+1)*math.tau/12
    p=(math.cos(a),math.sin(a));q=(math.cos(b),math.sin(b))
    face([(0,*p),(6,*p),(6,*q),(0,*q)],(0,math.cos((a+b)/2),math.sin((a+b)/2)))
    face([(0,0,0),(0,*p),(0,*q)],(-1,0,0))
    face([(6,0,0),(6,*q),(6,*p)],(1,0,0))
blob=bytearray();views=[];accessors=[]
def accessor(values,fmt,component,shape):
    blob.extend(bytes(-len(blob)%4))
    flat=[v for row in values for v in row] if shape!='SCALAR' else values
    data=struct.pack('<'+fmt*len(flat),*flat)
    views.append(dict(buffer=0,byteOffset=len(blob),byteLength=len(data)))
    blob.extend(data)
    accessors.append(dict(bufferView=len(views)-1,componentType=component,type=shape,count=len(values)))
    return len(accessors)-1
attributes=dict(POSITION=accessor(positions,'f',5126,'VEC3'),NORMAL=accessor(normals,'f',5126,'VEC3'),TEXCOORD_0=accessor(uvs,'f',5126,'VEC2'))
index=accessor(indices,'H',5123,'SCALAR')
write('shell.gltf',dict(asset=dict(version='2.0',generator='Aftershock original reference effects'),scene=0,scenes=[dict(nodes=[0])],
    nodes=[dict(mesh=0)],meshes=[dict(name='brass',primitives=[dict(attributes=attributes,indices=index,material=0)])],
    materials=[dict(name='brass',pbrMetallicRoughness=dict(baseColorFactor=[.55,.32,.08,1],metallicFactor=.85,roughnessFactor=.28),doubleSided=True)],
    buffers=[dict(uri='shell.bin',byteLength=len(blob))],bufferViews=views,accessors=accessors))
(out/'shell.bin').write_bytes(blob)

prefix='effects/reference/'
def emitter(name,texture,**fields):
    result=dict(name=name,kind='sprite',material=prefix+texture,capacity=64,rate=0,burst=1,lifetime_ms=1000,size=8)
    result.update(fields)
    return result
flash=emitter('flash','flash',lifetime_ms=90,size=14,end_size=3,color=[1,.85,.5,1],rotation_spread=180,
              light=dict(radius=180,intensity=3,color=[1,.65,.25]))
smoke=emitter('smoke','smoke',burst=16,lifetime_ms=1800,size=10,end_size=32,velocity=[0,0,18],velocity_spread=[8,8,8],
              origin_spread=[5,5,3],drag=.2,color=[.7,.72,.75,.35],rotation_spread=180,angular_velocity=12,
              flipbook=dict(columns=4,rows=2,fps=4),soft=True,lit=True)
sparks=emitter('sparks','spark',kind='trail',burst=24,lifetime_ms=900,size=1.2,end_size=.2,velocity=[100,0,40],
               velocity_spread=[60,140,140],gravity=[0,0,-240],drag=.25,collision=True,color=[1,.65,.18,1])
dust=emitter('dust','smoke',burst=18,lifetime_ms=1200,size=5,end_size=24,velocity=[25,0,8],velocity_spread=[20,35,20],
             color=[.62,.48,.3,.32],rotation_spread=180,angular_velocity=-8,flipbook=dict(columns=4,rows=2,fps=5),soft=True,lit=True)
shell=emitter('shell','flash',kind='mesh',model='models/reference_shell.iqm',material='models/reference_shell_material0',
              burst=1,lifetime_ms=1800,size=.65,velocity=[20,60,80],velocity_spread=[10,15,15],gravity=[0,0,-240],
              collision=True,rotation_spread=180,angular_velocity=700,lit=True)
fire=emitter('fire','fire',burst=10,lifetime_ms=650,size=24,end_size=46,origin_spread=[8,8,8],velocity_spread=[25,25,25],
             rotation_spread=180,flipbook=dict(columns=4,rows=2,fps=11),light=dict(radius=320,intensity=4,color=[1,.4,.08]))
tracer=emitter('tracer','spark',kind='trail',lifetime_ms=120,size=.6,end_size=.2,velocity=[2400,0,0],color=[1,.85,.5,1])
definitions=dict(muzzle=[flash],impact_metal=[dict(flash,name='glint',size=5,end_size=1),sparks],impact_stone=[dust],
                 smoke=[smoke],sparks=[sparks],dust=[dust],shell=[shell],explosion=[fire,dict(smoke,name='aftermath',burst=24),sparks],tracer=[tracer])
recipes=[dict(name='textures/'+prefix+name,kind='texture',source=name+'.png') for name in ('smoke','fire','flash','spark')]
recipes.append(dict(name='models/reference_shell',kind='model',source='shell.gltf',scale=1,material_model='metallic-roughness'))
for name,emitters in definitions.items():
    write(name+'.json',dict(version=1,name=name,emitters=emitters))
    recipes.append(dict(name=prefix+name,kind='effect',source=name+'.json'))
write('assets.json',dict(version=1,assets=recipes))
shaders=[]
for name in ('smoke','fire','flash','spark'):
    destination='GL_ONE_MINUS_SRC_ALPHA' if name=='smoke' else 'GL_ONE'
    shaders.append(f'{prefix}{name}\n{{\n cull disable\n {{\n  map textures/{prefix}{name}.ktx2\n  blendFunc GL_SRC_ALPHA {destination}\n  rgbGen vertex\n  alphaGen vertex\n }}\n}}\n')
(out/'reference.shader').write_text('\n'.join(shaders))
(out/'CREDITS.txt').write_text('Original procedural test artwork authored for Aftershock, 2026-09-21.\nCC0-1.0: https://creativecommons.org/publicdomain/zero/1.0/\nNo game content or third-party texture/model is included.\nexport.py records the original authoring recipe; CI verifies hashes and never reruns it.\n')
write('provenance.json',dict(version=1,license='CC0-1.0',author='Aftershock original procedural reference fixtures',
                           files={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.iterdir()) if p.is_file()}))
