#!/usr/bin/env python3
"""Compile owned polygon geometry, check physical brush occupancy and v1 identity."""
import argparse
import copy
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

from run import ROOT
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compile', action='store_true')
args = parser.parse_args()
fixture = ROOT/'tests/assets/levels'
legacy = json.loads((fixture/'two_lane.json').read_text())
level = {key:copy.deepcopy(legacy[key]) for key in ('materials','rules','props','pickups','lighting')}
level.update(version=2, name='polygon_test', props=[], pickups=[], lighting={'ambient':32,'lights':[]},
             boundary={'polygon':[[-640,-512],[640,-512],[640,512],[256,640],[-640,512]],
                       'holes':[[[-544,256],[-416,256],[-416,384],[-544,384]]], 'floor':0,'ceiling':768},
             shapes=[{'id':'building_7','kind':'building','shape':{'rectangle':{'center':[0,0],'size':[384,320],'angle':0}},
                      'base':0,'height':256,'wall_thickness':16,'floors':1,
                      'openings':[{'edge':0,'at':192,'width':96,'sill':0,'height':112}]},
                     {'id':'rotated','kind':'solid','shape':{'rectangle':{'center':[400,0],'size':[128,64],'angle':30}},'base':0,'height':96},
                     {'id':'circle','kind':'solid','shape':{'circle':{'center':[-400,0],'radius':48,'tolerance':1}},'base':0,'height':96},
                     {'id':'concave','kind':'solid','shape':{'polygon':[[-128,320],[128,320],[128,384],[0,384],[0,448],[-128,448]]},'base':0,'height':64}],
             spawns=[{'team':'ffa','origin':[0,-384,24],'angle':90},{'team':'ffa','origin':[320,384,24],'angle':180}])
level['rules'].update(max_sightline=4096,max_cover_gap=1024)


def brushes(text):
    # Read MAP planes independently of the generator; normals face brush interiors.
    result = []
    for block in re.findall(r'\{\s*((?:\([^\n]+\n)+)\}',text):
        planes = []
        for line in block.splitlines():
            a,b,c = [[float(v) for v in p.split()] for p in re.findall(r'\(\s*([^()]+)\)',line)]
            u,v = [[p[i]-a[i] for i in range(3)] for p in (b,c)]
            n = [u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
            planes.append((a,n))
        result.append(planes)
    assert result, 'compiler emitted no brushes'
    return result


def solid(planes,point):
    return any(all(sum((point[i]-a[i])*n[i] for i in range(3))>=-1e-4 for a,n in brush) for brush in planes)


with tempfile.TemporaryDirectory(prefix='aftershock-polygons-') as temporary:
    folder = Path(temporary)
    shutil.copytree(fixture/'assets',folder/'assets')
    source = folder/'level.json'
    def compile(document,name,fail=None,full=False):
        source.write_text(json.dumps(document))
        output = folder/name
        command = [sys.executable,str(ROOT/'tools/level'),str(source),'--output',str(output)]
        result = subprocess.run(command+([] if full else ['--map-only']),capture_output=True,text=True,cwd=ROOT)
        if fail:
            assert result.returncode and fail in result.stderr.lower(),(fail,result.stdout,result.stderr)
            return
        assert result.returncode==0,result.stderr
        report = json.loads(result.stdout)
        return {kind:(output/path).read_bytes() for kind in ('map','bsp','aas') if (path:=report.get(kind))}
    a = compile(level,'a',full=args.compile)
    b = compile(level,'b',full=args.compile)
    assert a==b, 'polygon compilation must be repeatable'
    planes = brushes(a['map'].decode())
    for point in ([0,0,48],[0,-160,48],[64,416,32],[-480,320,48],[-480,320,-8]):
        assert not solid(planes,point),('unexpected solid',point)
    for point in ([184,0,48],[0,0,250],[400,0,48],[-400,0,48],[-64,416,32],[-412,320,48]):
        assert solid(planes,point),('missing solid',point)
    changed = copy.deepcopy(level)
    changed['shapes'][0]['openings'] = []
    closed = brushes(compile(changed,'closed')['map'].decode())
    assert solid(closed,[0,-160,48]), 'door opening did not affect collision'
    changed = copy.deepcopy(level)
    changed['shapes'].extend([
        {'id':'terrace','kind':'platform','shape':{'rectangle':{'center':[0,-352],'size':[192,128]}},'base':0,'height':48},
        {'id':'steps','kind':'transition','shape':{'path':{'points':[[-288,-352],[-96,-352]],'thickness':96}},
         'base':0,'height':48,'transition':'stairs'},
        {'id':'ramp','kind':'transition','shape':{'path':{'points':[[96,-352],[288,-352]],'thickness':96}},
         'base':0,'height':48,'transition':'ramp','descending':True},
        {'id':'fence','kind':'wall','shape':{'path':{'points':[[384,-416],[512,-288]],'thickness':8}},
         'base':0,'height':96,'bullet_solid':False},
        {'id':'canopy','kind':'overhead','shape':{'rectangle':{'center':[-320,-352],'size':[256,192]}},'base':128,'height':16}])
    changed['spawns'][0]['origin'] = [0,-352,72]
    elevated = compile(changed,'elevated',full=args.compile)
    raised = brushes(elevated['map'].decode())
    for point in ([0,-352,24],[-112,-352,40],[112,-352,40],[-320,-352,136]):
        assert solid(raised,point),('missing raised surface',point)
    for point in ([-256,-352,40],[256,-352,40],[-320,-352,64]):
        assert not solid(raised,point),('blocked stair/ramp/overhead',point)
    assert b'level/fence' in elevated['map'], 'bullet-transparent fence needs its own collision material'
    changed = copy.deepcopy(level)
    changed['shapes'][0].update(floors=2,roof_access=True,
        opening_rules=[{'face_point':[0,-384],'spacing':128,'width':48,'sill':64,'height':40,'floors':[0,1]}])
    changed['spawns'].extend([{'team':'ffa','origin':[-144,0,152],'angle':0},
                              {'team':'ffa','origin':[-144,0,280],'angle':0}])
    storeys = compile(changed,'storeys',full=args.compile)
    upper = brushes(storeys['map'].decode())
    assert not solid(upper,[-128,-160,80]), 'rule-based street-facing window absent'
    assert solid(upper,[-144,0,120]), 'upper floor slab absent'
    assert not solid(upper,[0,-32,248]), 'roof stairwell must remain open'
    changed = copy.deepcopy(level)
    changed['shapes'][1]['id'] = 'building_7'
    compile(changed,'duplicate',fail='duplicate')
    changed = copy.deepcopy(level)
    changed['shapes'][2]['shape']['circle']['tolerance'] = 0
    compile(changed,'tolerance',fail='tolerance')
    changed = copy.deepcopy(level)
    changed['boundary']['polygon'] = [[-512,-512],[512,512],[-512,512],[512,-512]]
    compile(changed,'crossing',fail='polygon')
    changed = copy.deepcopy(level)
    changed['spawns'][0]['origin'] = [400,0,24]
    compile(changed,'spawn',fail='spawn')
    old = compile(legacy,'legacy',full=args.compile)
    for kind,data in old.items():
        assert data==(ROOT/'tests/golden/levels'/('two_lane.'+kind)).read_bytes(),('v1 changed',kind)
print('PASS: polygon holes, rotated/curved/concave solids, shell door collision, repeated output and unchanged v1')
