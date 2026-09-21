#!/usr/bin/env python3
"""Assemble the licensed reference kit, preserve clearances and stage native PBR."""
import argparse
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from run import ROOT
sys.path.insert(0,str(ROOT/'tools/level'))
from theme import assemble
from polygons import footprint,prop_bounds,opening_polygon
from tools.assets.manifest import validate
from shapely import Point,LineString

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--library',type=Path)
parser.add_argument('--modules',type=Path)
args=parser.parse_args()
with tempfile.TemporaryDirectory(prefix='aftershock-theme-level-') as temporary:
    root=Path(temporary)
    def run(command):
        result=subprocess.run([sys.executable,*map(str,command)],cwd=ROOT,capture_output=True,text=True)
        assert result.returncode==0,result.stderr
        return json.loads(result.stdout)
    library=args.library or root/'library'
    modules=args.modules or root/'modules'
    if not args.library:
        run(['tools/assets','fetch','--theme','manhattan','--out',library])
    if not args.modules:
        params=root/'parameters.json'
        params.write_text(json.dumps(dict(version=1,seed=164,bay_width=4,storey_height=4,wall_thickness=.5)))
        run(['tools/blender','kit','--parameters',params,'--out',modules])
    theme=json.loads((ROOT/'tools/level/themes/manhattan.json').read_text())
    level=dict(version=2,name='theme_test',materials={},rules=dict(min_corridor_width=64,min_door_height=80,max_sightline=8192,max_cover_gap=2048),
               boundary=dict(polygon=[[-1024,-768],[1024,-768],[1024,768],[-1024,768]],floor=0,ceiling=768),
               shapes=[dict(id='building_'+str(i),kind='building',shape=dict(rectangle=dict(center=[x,y],size=[384,384])),
                            base=0,height=128,wall_thickness=16,floors=1,
                            openings=[dict(edge=0,at=192,width=96,sill=0,height=96)])
                       for i,(x,y) in enumerate([(-576,256),(0,256),(576,256)])],
               spawns=[dict(team='ffa',origin=[-640,-512,24],angle=0),dict(team='ffa',origin=[640,-512,24],angle=180)],
               props=[],pickups=[],lighting=dict(ambient=32,lights=[]),
               intents=[dict(id='main_lane',kind='route',points=[[-900,-256],[900,-256]],width=128)])
    a=assemble(level,theme,library,modules,root/'a',seed=164)
    b=assemble(level,theme,library,modules,root/'b',seed=164)
    assert a==b and a['props'], 'theme placement must be seeded and repeatable'
    assert a['shapes']==level['shapes'], 'dressing changed authored geometry'
    for prop in a['props']:
        p,_,_=prop_bounds(prop)
        assert not p.intersects(LineString(level['intents'][0]['points']).buffer(64)), 'prop obstructs explicit lane'
        assert all(p.distance(Point(*s['origin'][:2]))>=theme['placement']['spawn_clearance'] for s in a['spawns'])
        for building in a['shapes']:
            shape=footprint(building['shape'])
            for door in building['openings']:
                assert not p.intersects(opening_polygon(shape,door,16).buffer(theme['placement']['door_clearance'])), 'prop obstructs doorway'
    validate(root/'a/assets')
    assert (root/'a/assets/CREDITS').is_file()
    assert (root/'a/assets/textures/theme/ground.asmat').is_file()
    report=run(['tools/level',root/'a/level.json','--output',root/'compiled','--map-only'])
    assert report['report']['reachable_spawns']==2
    assert (root/'compiled/textures/theme/ground.asmat').is_file(), 'PBR material lost during map staging'
    aliases=list((root/'compiled/textures/s').rglob('*.asmat'))
    assert aliases, 'source-labeled brush materials lost their PBR bindings'
    runtime_shaders=(root/'compiled/scripts/level.shader').read_text()
    assert 'textures/s/' not in runtime_shaders, 'compiler-only shader aliases shadow native PBR at runtime'
print('PASS: licensed seeded theme assembly, unchanged shapes, door/lane/spawn clearance and native PBR staging')
