#!/usr/bin/env python3
"""Assemble the licensed reference kit, preserve clearances and stage native PBR."""
import argparse
import copy
import json
from pathlib import Path
import subprocess
import shutil
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
parser.add_argument('--client',type=Path)
parser.add_argument('--server',type=Path)
parser.add_argument('--output',type=Path)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
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
    level['pickups']=[dict(classname=name,origin=point) for name,point in [
        ('weapon_rocketlauncher',[-512,-320,16]),('weapon_lightning',[512,-320,16]),
        ('weapon_railgun',[0,0,16]),('ammo_rockets',[-576,256,16]),('ammo_lightning',[576,256,16]),
        ('item_armor_combat',[0,-512,16]),('item_health',[0,256,16])]]
    level['viewpoints']=[dict(id='facades',origin=[0,-128,72],angles=[0,90,0]),
                         dict(id='street',origin=[-768,-64,72],angles=[0,20,0]),
                         dict(id='modules',origin=[0,640,72],angles=[0,-90,0])]
    a=assemble(level,theme,library,modules,root/'a',seed=164)
    b=assemble(level,theme,library,modules,root/'b',seed=164)
    assert a==b and a['props'], 'theme placement must be seeded and repeatable'
    assert a['shapes']==level['shapes'], 'dressing changed authored geometry'
    assert any(p['material']=='kit_facade' for p in a['props']), 'facade modules must retain their baked material'
    assert any(p['material']=='prop' for p in a['props']), 'ground dressing was silently omitted'
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
    report=run(['tools/level',root/'a/level.json','--output',root/'compiled']+([] if args.client else ['--map-only']))
    assert report['report']['reachable_spawns']==2
    assert ' 0 0 0 0.0625 0.0625 ' in (root/'compiled/maps/theme_test.map').read_text(), 'theme texture scale ignored'
    assert (root/'compiled/textures/theme/ground.asmat').is_file(), 'PBR material lost during map staging'

    if args.client:
        from overhead import triangles
        vertices=[v for shader,tri in triangles((root/'compiled/maps/theme_test.bsp').read_bytes())
                  if shader=='textures/theme/kit_facade' for v in tri]
        assert vertices and abs(max(v[2] for v in vertices)-128)<.01, 'compiled facade axes differ from declared native bounds'
    aliases=list((root/'compiled/textures/s').rglob('*.asmat'))
    assert aliases, 'source-labeled brush materials lost their PBR bindings'
    runtime_shaders=(root/'compiled/scripts/level.shader').read_text()
    assert 'textures/s/' not in runtime_shaders, 'compiler-only shader aliases shadow native PBR at runtime'
    if args.client or args.server:
        assert args.client and args.server and args.output, 'native acceptance requires client, server and retained output'
        from tools.agent import Engine
        with Engine(args.client,args.data,args.content) as engine:
            shutil.copytree(root/'compiled',engine.base,dirs_exist_ok=True)
            engine.request('session',dt=20,seed=164)
            engine.request('map',name='theme_test')
            engine.step(150)
            clear=engine.request('trace',start=[0,-128,48],end=[0,100,48],hull='point')
            blocked=engine.request('trace',start=[150,-128,48],end=[150,100,48],hull='point')
            floor=engine.request('trace',start=[-640,-512,64],end=[-640,-512,-16],hull='player')
            assert clear['fraction']==1 and not clear['start_solid'],clear
            assert blocked['fraction']<1 and not blocked['start_solid'],blocked
            assert floor['fraction']<1 and abs(floor['end'][2]-24)<.2 and floor['normal']==[0,0,1],floor
            args.output.mkdir(parents=True,exist_ok=True)
            (args.output/'agent-traces.json').write_text(json.dumps(dict(clear=clear,blocked=blocked,floor=floor),indent=2)+'\n')
        native=run(['tools/level','validate',root/'a/level.json','--output',args.output,
                    '--client',args.client,'--server',args.server,'--content',args.content,'--data',args.data])
        assert native['status']=='passed' and native['bots']['samples']>=100,native
        assert native['bots']['kills']>=2 and native['bots']['stuck']==[],native['bots']
        assert native['views'] or native['flythrough'], 'native theme captures absent'
        log=(args.output/'client.log').read_text(errors='replace')
        assert 'Invalid or unavailable cooked material' not in log and 'Failed to load model' not in log

print('PASS: licensed seeded theme assembly, unchanged shapes, door/lane/spawn clearance and native PBR staging')
