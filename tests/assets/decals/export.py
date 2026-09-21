#!/usr/bin/env python3
"""Author original decal color/height-derived normals once; CI never runs this."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random

from PIL import Image

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
if any(path.name!='export.py' for path in out.iterdir()):
    raise SystemExit('refusing to overwrite an authored reference set')
(out/'export.py').write_bytes(Path(__file__).read_bytes())
def write(name,data):
    (out/name).write_text(json.dumps(data,indent=2)+'\n')
def clamp(value):return max(0,min(1,value))
def byte(value):return round(255*clamp(value))
side=256
rng=random.Random(161)
drops=[(rng.uniform(-.65,.65),rng.uniform(-.65,.65),rng.uniform(.025,.13)) for _ in range(28)]
recipes=[]
for name in ('bullet','scorch','blood'):
    colors=[];heights=[]
    for y in range(side):
        for x in range(side):
            u,v=(x+.5)/side*2-1,(y+.5)/side*2-1
            r=math.hypot(u,v);a=math.atan2(v,u)
            noise=.5+.25*math.sin(79*u+math.sin(93*v))+.25*math.sin(123*v+31*u)
            rough=r*(1+.065*math.sin(13*a)+.04*math.sin(21*a))
            if name=='bullet':
                # Recessed center, chipped raised rim, faint powder around it.
                cavity=clamp((.35-rough)*40)
                rim=math.exp(-((rough-.38)/.065)**2)
                dust=math.exp(-((r-.42)/.16)**2)*(.1+.15*noise)
                alpha=max(cavity,rim,dust)*clamp((.82-r)*10)
                shade=.07+.42*rim*(.55+.45*noise)
                color=(shade,shade*.93,shade*.82)
                height=-.4*cavity+.15*rim
            elif name=='scorch':
                streak=.65+.35*(.5+.5*math.sin(17*a+math.sin(7*a)))**3
                alpha=clamp((.92-r)*2)*streak*(.65+.35*noise)
                shade=.045+.09*noise+.08*r
                color=(shade,shade*.65,shade*.4)
                height=-.025*alpha*(.6+.4*noise)
            else:
                blob=math.exp(-((u*1.2)**2+(v*.85)**2)*11)
                for dx,dy,radius in drops:
                    blob=max(blob,clamp((radius-math.hypot(u-dx,v-dy))*120))
                alpha=clamp(blob*2.5)*clamp((.94-r)*20)
                shade=.2+.16*noise+.06*blob
                color=(shade,shade*.055,shade*.025)
                height=.045*alpha
            colors.append(tuple(byte(c) for c in color)+(byte(alpha),));heights.append(height)
    image=Image.new('RGBA',(side,side));image.putdata(colors);image.save(out/(name+'.png'),compress_level=9)
    normals=[]
    for y in range(side):
        for x in range(side):
            dx=(heights[y*side+min(x+1,side-1)]-heights[y*side+max(0,x-1)])*side/4
            dy=(heights[min(y+1,side-1)*side+x]-heights[max(0,y-1)*side+x])*side/4
            length=math.sqrt(dx*dx+dy*dy+1)
            normals.append((byte(.5-dx/length*.5),byte(.5-dy/length*.5),byte(.5+.5/length)))
    normal=Image.new('RGB',(side,side));normal.putdata(normals);normal.save(out/(name+'_normal.png'),compress_level=9)
    prefix='textures/decals/reference/'+name
    recipes.extend([dict(name=prefix,kind='texture',source=name+'.png',srgb=True),
                    dict(name=prefix+'_normal',kind='texture',source=name+'_normal.png',format='bc5',normal=True)])
    width={'bullet':12,'scorch':96,'blood':48}[name]
    write(name+'.json',dict(version=1,name=name,color_map=prefix+'.ktx2',normal_map=prefix+'_normal.ktx2',
                           size=[width,width,8],lifetime_ms=15000,fade_ms=3000,color=[1,1,1,1],normal_strength=1))
    recipes.append(dict(name='decals/reference/'+name,kind='decal',source=name+'.json'))
write('assets.json',dict(version=1,assets=recipes))
(out/'CREDITS.txt').write_text('Original procedural test artwork authored for Aftershock, 2026-09-21.\nCC0-1.0: https://creativecommons.org/publicdomain/zero/1.0/\nNo game content or third-party artwork. export.py records initial authoring; CI never regenerates it.\n')
write('provenance.json',dict(version=1,license='CC0-1.0',author='Aftershock original procedural reference fixtures',
                           files={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.iterdir()) if p.is_file()}))
