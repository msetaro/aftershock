#!/usr/bin/env python3
"""Measure an owned playbook drawing, preserve its key and stable edit decisions."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw
from run import ROOT

with tempfile.TemporaryDirectory(prefix='aftershock-sketch-') as temporary:
    root = Path(temporary)
    drawing = Image.new('RGB',(640,480),(246,243,234))
    pen = ImageDraw.Draw(drawing)
    red,blue,green = '#b52e34','#245eae','#29713b'
    pen.line([(22,20),(620,24),(616,460),(20,455),(22,20)],fill='#222222',width=3)
    shapes = [[(72,94),(182,86),(194,192),(82,200)],[(264,84),(366,86),(360,190),(266,192)],
              [(432,240),(544,242),(540,352),(430,348)]]
    for points in shapes:
        # Wobbly outline, small unclosed gap and crosshatching share one pen color.
        pen.line(points+[points[0]],fill=red,width=3)
        x,y = points[0]
        pen.line([(x+25,y+25),(x+65,y+70)],fill=red,width=2)
        pen.line([(x+50,y+20),(x+80,y+55)],fill=red,width=2)
    pen.line([(120,90),(125,90)],fill=(246,243,234),width=7)
    pen.line([(50,320),(230,240),(450,170)],fill=blue,width=5)
    pen.line([(425,168),(450,170),(434,190)],fill=blue,width=5)
    pen.line([(50,400),(380,400)],fill=blue,width=2)
    pen.text((170,210),'8 seconds to B',fill=green)
    pen.rectangle((390,30,604,120),outline=green,width=2)
    pen.text((400,42),'KEY: red = enterable',fill=green)
    pen.text((400,62),'blue = route / sight',fill=green)
    pen.text((400,82),'hatch = 2 floors',fill=green)
    image = root/'playbook.png'
    drawing.save(image)
    notes = dict(version=1,name='sketch_test',theme='manhattan',seed=164,
                 scale=dict(pixels=100,units=256),boundary=[[22,20],[620,24],[616,460],[20,455]],
                 key=[dict(color=red,classification='geometry',kind='building',floors=2,reading='red enterable; hatch two floors'),
                      dict(color=blue,classification='intent',kind='route',reading='blue routes and sightlines'),
                      dict(color=green,classification='annotation',kind='text',reading='green annotations')],
                 marks=[dict(id='key',classification='annotation',region=[390,30,604,120],text='red enterable; blue route/sight; hatch two floors',confidence=1),
                        dict(id='timing',classification='annotation',region=[160,202,280,232],text='8 seconds to B',target='route_a',confidence=.95),
                        dict(id='route_a',classification='intent',kind='route',points=[[50,320],[230,240],[450,170]],target_seconds=8,confidence=.95),
                        dict(id='sight_a',classification='intent',kind='sightline',points=[[50,400],[380,400]],blocked=False,confidence=.95),
                        dict(id='building_7',classification='geometry',region=[62,76,202,210],color=red,confidence=.95)],
                 overrides={},gap_pixels=7,simplify_pixels=2)
    path = root/'notes.json'
    def trace(value,out):
        path.write_text(json.dumps(value))
        result = subprocess.run([sys.executable,'tools/level','trace','--sketch',str(image),'--notes',str(path),
                                 '--out',str(out)],cwd=ROOT,capture_output=True,text=True)
        assert result.returncode==0,result.stderr
        summary = json.loads(result.stdout)
        assert summary['ok'],summary
        interpretation = json.loads((out/'interpretation.json').read_text())
        level = json.loads((out/'level.json').read_text())
        assert (out/'interpretation.png').is_file() and (out/'overlay.png').is_file()
        return interpretation,level
    first,level = trace(notes,root/'result')
    assert len(level['shapes'])==3, 'arrows, handwriting or hatch strokes became geometry'
    assert all(s['kind']=='building' and s['floors']==2 for s in level['shapes']), 'drawing key ignored'
    assert any(s['id']=='building_7' for s in level['shapes']), 'explicit reference ID lost'
    assert {m['classification'] for m in first['marks']}=={'geometry','annotation','intent'}
    assert {m['id'] for m in first['marks']} >= {'key','timing','route_a','sight_a','building_7'}
    assert all('reading' in m and 'confidence' in m for m in first['marks'])
    assert {i['id'] for i in level['intents']}=={'route_a','sight_a'}
    changed = copy.deepcopy(notes)
    changed['overrides']['building_7'] = dict(floors=1,height=160)
    _,edited = trace(changed,root/'result')
    a,b = [{s['id']:s for s in v['shapes']} for v in (level,edited)]
    assert a.keys()==b.keys() and all((a[k]!=b[k])==(k=='building_7') for k in a)
    changed = copy.deepcopy(notes)
    changed['key'][0].update(kind='solid',floors=1,reading='red is solid in this drawing')
    _,rekeyed = trace(changed,root/'rekeyed')
    assert all(s['kind']=='solid' for s in rekeyed['shapes']), 'fixed global legend overrode drawing key'
    # A mark whose color is known still needs an explicit numbered interpretation.
    pen.ellipse((270,275,310,315),outline=blue,width=3)
    pen.text((310,320),'?',fill=green)
    drawing.save(image)
    unmatched,_ = trace(notes,root/'unmatched')
    assert any(m['id'].startswith('unread_') for m in unmatched['marks']), 'known-color marks silently discarded'
    assert any(a['id'].startswith('unread_') for a in unmatched['assumptions']), 'unread marks need an assumption'
    changed = copy.deepcopy(notes)
    changed['marks'][2].update(confidence=.3,reading='arrow might be a ramp')
    uncertain,_ = trace(changed,root/'ambiguous')
    assert any(a['id']=='route_a' for a in uncertain['assumptions']), 'ambiguity silently guessed'
print('PASS: owned multicolor stroke drawing, authoritative key, annotation separation, intent records and stable edits')

# A second owned drawing covers touching outlines, narrow/dashed marks and a
# dominant rotated grid. Explicit regions are the agent's semantic split decision.
import math
import cv2
import numpy as np
from shapely import Polygon
from tools.level.sketch import measure
with tempfile.TemporaryDirectory(prefix='aftershock-sketch-measure-') as temporary:
    image = Image.new('RGB',(640,480),'#181b20')
    pen = ImageDraw.Draw(image)
    color = '#d5d0c3'
    pen.rectangle((40,60,150,160),outline=color,width=3)
    pen.rectangle((150,60,240,130),outline=color,width=3)
    for x in range(300,550,22):
        pen.line((x,90,x+13,90),fill='#b98c41',width=2)
    pen.line((300,140,540,170),fill='#507fb0',width=2)
    for start in range(10,170,20):
        pen.arc((330,200,530,400),start,start+12,fill='#7ba47a',width=4)
    def rectangle(cx,cy,angle):
        a=math.radians(angle)
        return [(cx+x*math.cos(a)-y*math.sin(a),cy+x*math.sin(a)+y*math.cos(a))
                for x,y in ((-55,-35),(55,-35),(55,35),(-55,35))]
    pen.polygon(rectangle(90,300,22),fill='#ba5555')
    pen.polygon(rectangle(230,325,24),fill='#ba5555')
    notes=dict(version=1,scale=dict(player_height_pixels=28,player_height_units=56),
               boundary=[[10,10],[630,10],[630,470],[10,470]],gap_pixels=15,
               snap_degrees=5,
               key=[dict(color=color,classification='geometry',kind='building',line_style='outline'),
                    dict(color='#b98c41',classification='geometry',kind='overhead',line_style='dashed',base=128,height=16),
                    dict(color='#507fb0',classification='geometry',kind='wall',line_style='thin',height=64),
                    dict(color='#7ba47a',classification='geometry',kind='overhead',line_style='dashed',base=160,height=16),
                    dict(color='#ba5555',classification='geometry',kind='solid',line_style='filled')],
               marks=[dict(id='building_1',classification='geometry',region=[38,58,150,162],color=color,confidence=1),
                      dict(id='annex',classification='geometry',region=[150,58,242,132],color=color,confidence=1)])
    interpretation,level,_=measure(image,notes)
    assert len(level['shapes'])==7, [(s['id'],s['kind']) for s in level['shapes']]
    assert {'building_1','annex'}<={s['id'] for s in level['shapes']}, 'touching regions were merged'
    assert interpretation['scale']==2
    assert any(18<a<28 for a in interpretation['dominant_angles_degrees']), interpretation
    assert all(m.get('line_style') for m in interpretation['marks'] if m.get('pixel_polygon'))
    solids=[s for s in level['shapes'] if s['kind']=='solid']
    def angle(s):
        points=s['shape']['polygon'];a,b=points[:2]
        return math.degrees(math.atan2(b[1]-a[1],b[0]-a[0]))%90
    assert abs(angle(solids[0])-angle(solids[1]))<.01, 'nearby grid angles did not snap'
    # Simulate a phone photograph with a dark UI border, then rectify it using
    # agent-read page corners. Mark coordinates remain in rectified image space.
    source=np.float32([[0,0],[639,0],[639,479],[0,479]])
    corners=np.float32([[45,35],[670,65],[640,510],[20,475]])
    transform=cv2.getPerspectiveTransform(source,corners)
    photo=Image.fromarray(cv2.warpPerspective(np.array(image),transform,(720,550),borderValue=(12,12,12)))
    photographed=copy.deepcopy(notes)
    photographed.update(corners=corners.tolist(),rectified_size=[640,480])
    measured,other,_=measure(photo,photographed)
    assert measured['perspective'] and len(other['shapes'])==7
    for a,b in zip(level['shapes'],other['shapes']):
        pa,pb=Polygon(a['shape']['polygon']),Polygon(b['shape']['polygon'])
        assert pa.intersection(pb).area/pa.union(pb).area>.65, (a['id'],b['id'])
print('PASS: touching-region split, thin/dashed line and curve, dark drawing, dominant grid and perspective correction')
