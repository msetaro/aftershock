#!/usr/bin/env python3
"""Check shooter reports against a controlled world through the agent trace contract."""
import copy
import json
import math
from pathlib import Path
import sys

import shapely
from shapely import Point,LineString,box
from run import ROOT
sys.path.insert(0,str(ROOT/'tools/level'))
from intents import analyze

level=dict(version=2,name='intent_test',materials={r:'level/'+r for r in ('floor','wall','trim','cover','prop','sky')},
           boundary=dict(polygon=[[-512,-512],[512,-512],[512,512],[-512,512]],floor=0,ceiling=256),
           rules=dict(min_corridor_width=64,min_door_height=80,max_sightline=2048,max_cover_gap=1024),
           shapes=[dict(id='screen',kind='solid',shape=dict(rectangle=dict(center=[0,0],size=[48,256])),base=0,height=128)],
           spawns=[dict(team='red',origin=[-256,0,24],angle=0),dict(team='blue',origin=[256,0,24],angle=180)],
           props=[],pickups=[],lighting=dict(ambient=32,lights=[]),
           intents=[dict(id='screen_blocks',kind='sightline',points=[[-256,0],[256,0]],blocked=True),
                    dict(id='south_route',kind='route',points=[[-256,0],[-256,-256],[256,-256],[256,0]],width=64),
                    dict(id='hold_west',kind='hold',points=[[-256,-256]],radius=128)])

class World:
    def __init__(self):
        self.calls=0
    def request(self,op,**fields):
        assert op=='trace'
        self.calls+=1
        start,end=fields['start'],fields['end']
        hull=15 if fields['hull']=='player' else 0
        walls=box(-24,-128,24,128).buffer(hull).union(box(-600,-600,600,600).difference(box(-496,-496,496,496)).buffer(hull))
        a,b=Point(*start[:2]),Point(*end[:2])
        segment=LineString([a,b])
        fraction=1
        inside=walls.contains(a)
        if inside:
            fraction=0
        elif segment.length and walls.intersects(segment):
            hit=walls.intersection(segment)
            fraction=min(segment.project(Point(*p))/segment.length for p in shapely.get_coordinates(hit))
        return dict(fraction=fraction,start_solid=inside,all_solid=inside,end=[a+(b-a)*fraction for a,b in zip(start,end)],normal=[0,0,1],contents=int(fraction<1),surface_flags=0)

world=World()
report=analyze(world,level)
assert report['passed'],report
assert world.calls==report['trace_queries'] and world.calls>10
assert report['lanes'] and report['sightlines']['samples']>0
assert sum(report['sightlines']['histogram'])==report['sightlines']['samples']
assert 0<=report['cover_density']<=1
assert report['first_contact'] and report['first_contact'][0]['estimated_seconds']>0
assert {i['id'] for i in report['intents']}=={'screen_blocks','south_route','hold_west'}
assert next(i for i in report['intents'] if i['id']=='south_route')['playtest_required']
changed=copy.deepcopy(level)
changed['intents'][0]['blocked']=False
bad=analyze(World(),changed)
assert not bad['passed'] and any(e['code']=='intent_sightline' for e in bad['errors']),bad
changed=copy.deepcopy(level)
changed['spawns'][0]['origin'][1]=-300
changed['spawns'][1]['origin'][1]=-300
bad=analyze(World(),changed)
assert not bad['passed'] and any(e['code']=='spawn_visibility' for e in bad['errors']),bad
changed=copy.deepcopy(level)
changed['intents'].append(dict(id='flag',kind='objective',points=[[-256,128]],team='red',mode='ctf'))
bad=analyze(World(),changed)
assert any(e['code']=='objective_visibility' for e in bad['errors'])
assert all(e['suggestion'] for e in bad['errors']),bad
print('PASS: agent-traced spawn/objective sightlines, explicit intent controls, lanes, cover and contact estimates')
