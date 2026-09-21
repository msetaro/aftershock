#!/usr/bin/env python3
# SPDX-License-Identifier: CC0-1.0
"""Explicit authoring of the original sketch reference: run with --write outside CI."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path

from PIL import Image,ImageDraw

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--write',action='store_true')
args=parser.parse_args()
if not args.write or os.environ.get('CI'):
    parser.error('fixture authoring requires --write and is forbidden in CI')
root=Path(__file__).resolve().parent
image=Image.new('RGB',(800,640),(246,243,234))
pen=ImageDraw.Draw(image)
red,orange,purple,blue,green,cyan='#b52e34','#d09b31','#775ca8','#245eae','#29713b','#278e9b'
boundary=[[25,40],[750,30],[775,570],[620,610],[35,595]]
pen.line([tuple(p) for p in boundary]+[tuple(boundary[0])],fill='#222222',width=3)
buildings=[('building_1',[[70,90],[220,90],[220,240],[70,240]]),
           ('building_7',[[325,90],[475,90],[475,240],[325,240]])]
angle=math.radians(15)
buildings.append(('building_9',[[645+x*math.cos(angle)-y*math.sin(angle),170+x*math.sin(angle)+y*math.cos(angle)]
                                for x,y in [(-70,-75),(70,-75),(70,75),(-70,75)]]))
marks=[]
for name,points in buildings:
    pen.line([tuple(p) for p in points]+[tuple(points[0])],fill=red,width=3)
    x=sum(p[0] for p in points)/4;y=sum(p[1] for p in points)/4
    pen.line([(x-35,y-35),(x+30,y+35)],fill=red,width=2)
    pen.text((x-16,y-8),name.split('_')[1],fill=green)
    marks.append(dict(id=name,classification='geometry',color=red,
                      region=[int(min(p[0] for p in points))-4,int(min(p[1] for p in points))-4,
                              int(max(p[0] for p in points))+5,int(max(p[1] for p in points))+5],
                      reading='Red outline is an enterable building; numeral is its stable ID',confidence=1))
    marks.append(dict(id='label_'+name,classification='annotation',region=[int(x-20),int(y-12),int(x+20),int(y+12)],text=name,confidence=1))
pen.ellipse((158,418,222,482),fill=orange)
a=math.radians(-25)
pen.polygon([(350+x*math.cos(a)-y*math.sin(a),470+x*math.sin(a)+y*math.cos(a)) for x,y in [(-50,-23),(50,-23),(50,23),(-50,23)]],fill=orange)
for start in range(205,350,18):
    pen.arc((515,375,685,545),start,start+12,fill=purple,width=4)
pen.line((70,310,715,310),fill=blue,width=4)
pen.line([(695,300),(715,310),(695,320)],fill=blue,width=4)
pen.ellipse((365,345,435,415),outline=blue,width=2)
pen.text((300,292),'6 seconds',fill=green)
marks.extend([dict(id='main_route',classification='intent',kind='route',points=[[70,310],[715,310]],
                   width=128,target_seconds=6,tolerance_seconds=2,reading='Arrow is a timed walking route',confidence=1),
              dict(id='plaza_hold',classification='intent',kind='hold',points=[[400,380]],radius=90,
                   region=[362,342,438,418],reading='Blue circle is a hold zone, not a building',confidence=1),
              dict(id='timing',classification='annotation',region=[295,286,380,307],text='6 seconds',target='main_route',confidence=1)])
for name,point,yaw in [('west_spawn',[115,145],0),('east_spawn',[675,150],180)]:
    x,y=point
    pen.ellipse((x-6,y-6,x+6,y+6),fill=cyan)
    marks.append(dict(id=name,classification='intent',kind='spawn',points=[point],team='ffa',angle=yaw,
                      reading='Cyan dot is a player spawn inside the building',confidence=1))
pen.rectangle((180,8,620,67),fill=(246,243,234),outline=green,width=2)
pen.text((190,15),'KEY red: enterable / orange: low cover / purple dash: wall',fill=green)
pen.text((190,33),'blue arrow: route / blue circle: hold / cyan dot: spawn',fill=green)
pen.text((190,50),'100 pixels = 256 units; numbered buildings keep their IDs',fill=green)
marks.append(dict(id='key',classification='annotation',region=[177,5,624,70],text='Drawing key and stated scale',confidence=1))
def world(p,z):
    return [round((p[0]-400)*2.56),round((320-p[1])*2.56),z]
notes=dict(version=1,name='sketch_reference',theme='manhattan',seed=164,mode='ffa',
           scale=dict(pixels=100,units=256),boundary=boundary,gap_pixels=15,simplify_pixels=2,
           key=[dict(color=red,classification='geometry',kind='building',line_style='outline',height=128),
                dict(color=orange,classification='geometry',kind='solid',line_style='filled',height=48),
                dict(color=purple,classification='geometry',kind='wall',line_style='dashed',height=96),
                dict(color=blue,classification='intent',kind='route'),dict(color=cyan,classification='intent',kind='spawn'),
                dict(color=green,classification='annotation',kind='text')],marks=marks,overrides={},
           pickups=[dict(classname=name,origin=world(p,16)) for name,p in [
               ('weapon_rocketlauncher',[140,300]),('weapon_lightning',[650,300]),('weapon_railgun',[400,335]),
               ('ammo_rockets',[145,165]),('ammo_lightning',[645,170]),('item_armor_combat',[400,380]),('item_health',[400,165])]],
           viewpoints=[dict(id='plaza',origin=world([400,285],72),angles=[0,90,0]),
                       dict(id='street',origin=world([50,300],72),angles=[0,20,0])],
           free_text='Three enterable buildings around a plaza; keep the blue route open. Original Aftershock test layout, not a commercial map.')
image.save(root/'reference.png')
(root/'notes.json').write_text(json.dumps(notes,sort_keys=True,indent=2)+'\n')
def record(name):
    return dict(path=name,sha256=hashlib.sha256((root/name).read_bytes()).hexdigest())
manifest=dict(version=1,assets=[dict(id='aftershock_sketch_reference',author='Aftershock',license='CC0-1.0',
    source_url='https://github.com/msetaro/aftershock/blob/main/tests/assets/sketch/export.py',retrieved='2026-09-21',
    attribution='Original Aftershock reference drawing and structured agent reading',originals=[record('export.py')],
    cooked=[record('reference.png'),record('notes.json')])])
(root/'manifest.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
