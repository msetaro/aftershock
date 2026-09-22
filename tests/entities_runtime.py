#!/usr/bin/env python3
"""Verify map/runtime prefab spawning and edited quantities through real pickups."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from cook import cook
from run import ROOT, SCRATCH
sys.path.insert(0,str(ROOT))
from tools.agent import Engine

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary',type=Path,required=True)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-entities-runtime')
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-entities-runtime-') as temporary:
    root=Path(temporary)
    source=root/'source'
    source.mkdir()
    definition=json.loads((ROOT/'tests/assets/entities/pickups.json').read_text())
    # The existing mega-health component allows observable gains above 100.
    definition['definitions'] += [
        dict(id='medical_boost',extends='medical_crate',native='item_health_mega',components={}),
        dict(id='medical_boost_small',extends='medical_boost',components=dict(pickup=dict(amount=15)))]
    (source/'pickups.json').write_text(json.dumps(definition))
    project=source/'assets.json'
    project.write_text(json.dumps(dict(version=1,assets=[dict(name='entities/pickups',kind='entities',source='pickups.json')])))
    home=root/'home'
    base=home/('baseoa' if args.content=='openarena' else 'baseq3')
    with (args.output/'compile.json').open('w') as log:
        subprocess.run([sys.executable,'tools/level',str(ROOT/'tests/assets/levels/two_lane.json'),'--output',str(base)],
                       cwd=ROOT,stdout=log,check=True,timeout=600)
    cook(project,base)
    (base/'maps/prefab-test.ent').write_text('''{
"classname" "worldspawn"
}
{
"classname" "info_player_deathmatch"
"origin" "-160 -160 48"
"angle" "0"
}
{
"classname" "medical_boost"
"origin" "128 0 48"
"count" "45"
"targetname" "map_crate"
}
''')
    with Engine(args.binary,args.data,args.content,home=home,
                arguments=['+set','g_entityDefinitions','entities/pickups.asent']) as engine:
        try:
            engine.request('session',dt=20,seed=18)
            engine.request('cvar.set',name='dev_entityFile',value='maps/prefab-test.ent')
            engine.request('cvar.set',name='dev_loadEntities',value='1')
            engine.request('map',name='two_lane')
            engine.step(50)
            rows=engine.request('entity.list')['entities']
            crates=[row for row in rows if row['classname']=='medical_boost']
            assert len(crates)==1, 'map classname must resolve through the cooked prefab'
            entity=crates[0]['entity']
            assert engine.request('entity.get',entity=entity,key='count')['value']=='45', 'map instance overrides inherited amount'
            def execute(command):
                engine.request('exec',command=command)
                engine.step(3)
            def collect(x,y,amount):
                execute('give health')
                assert engine.request('state')['player']['health']==100
                execute(f'setviewpos {x} {y} 32 0')
                engine.step(3)
                health=engine.request('state')['player']['health']
                # Existing overhealth decays by one at a one-second boundary.
                assert health in (100+amount,99+amount),(amount,health)
            collect(128,0,45)
            execute('setviewpos -160 -160 32 0')
            entity=engine.request('entity.spawn',classname='medical_boost_small',x=128,y=128,z=48)['entity']
            engine.step(20)
            assert engine.request('entity.get',entity=entity,key='count')['value']=='15'
            engine.request('entity.set',entity=entity,key='count',value='20')
            assert engine.request('entity.get',entity=entity,key='count')['value']=='20'
            collect(128,128,20)
            print('PASS: map prefab/instance override, runtime inherited pickup and generic field editing affect real health')
        finally:
            shutil.copyfile(engine.log_path,args.output/'engine.log')
