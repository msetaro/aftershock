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
    # A targetname makes a classic pickup dormant until triggered.
    definition['definitions'][1]['components'].pop('hooks')
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
                assert health in (100+amount,99+amount),(amount,health,engine.request('state'),engine.request('entity.list'))
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
    # Reuse the owned animated character; no source fixture is regenerated.
    for name in ('character.gltf','character.bin','character.png'):
        shutil.copyfile(ROOT/'tests/assets/cook-character'/name,source/name)
    import math
    import struct
    import wave
    with wave.open(str(source/'hum.wav'),'wb') as sound:
        sound.setparams((1,2,48000,0,'NONE','not compressed'))
        sound.writeframes(b''.join(struct.pack('<h',int(4000*math.sin(2*math.pi*220*i/48000))) for i in range(4800)))
    composed=json.loads((ROOT/'tests/assets/entities/composed.json').read_text())
    prop=composed['definitions'][0]
    prop['components']['transform']=dict(origin=[0,0,0],angles=[0,0,0])
    prop['components']['animation']['first_frame']=31
    prop['components']['collision']=dict(mins=[-16,-16,0],maxs=[16,16,64],solid=True)
    prop['components'].pop('trigger')
    prop['components']['damage']['splash']=0
    composed['definitions']+=definition['definitions']+[dict(id='touch_zone',native='composed',components=dict(
        collision=dict(mins=[-32,-32,-32],maxs=[32,32,32],solid=False),
        trigger=dict(wait_ms=1000,once=True),damage=dict(amount=7),hooks=dict(target='door_signal')))]
    (source/'composed.json').write_text(json.dumps(composed))
    project.write_text(json.dumps(dict(version=1,assets=[
        dict(name='models/character',kind='model',source='character.gltf',scale=32,fps=30),
        dict(name='sound/entities/hum',kind='audio',source='hum.wav'),
        dict(name='entities/composed',kind='entities',source='composed.json')])))
    cook(project,base)
    (base/'maps/composed-test.ent').write_text('''{
"classname" "worldspawn"
}
{
"classname" "info_player_deathmatch"
"origin" "-160 -160 48"
}
{
"classname" "signal_crate"
}
{
"classname" "touch_zone"
"origin" "-128 128 32"
}
{
"classname" "medical_boost"
"origin" "128 128 48"
"targetname" "door_signal"
}
''')
    with Engine(args.binary,args.data,args.content,home=home,arguments=[
            '+set','g_entityDefinitions','entities/composed.asent','+set','cg_draw2D','0','+set','cg_drawGun','0','+set','con_notifytime','0']) as engine:
        try:
            engine.request('session',dt=20,seed=18)
            engine.request('cvar.set',name='dev_entityFile',value='maps/composed-test.ent')
            engine.request('cvar.set',name='dev_loadEntities',value='1')
            engine.request('map',name='two_lane')
            engine.step(50)
            def entity(classname):
                return next((row for row in engine.request('entity.list')['entities'] if row['classname']==classname),None)
            prop=entity('signal_crate')
            assert prop and prop['linked'], 'composed native entity must spawn'
            assert prop['model']>0 and prop['sound']>0 and prop['contents']&1 and prop['health']==20,prop
            assert not entity('medical_boost')['linked'], 'target pickup starts dormant'
            execute('team spectator')
            execute('dev_view -128 0 48 0 0 0')
            engine.step(4)
            from PIL import Image, ImageChops
            def capture(name):
                result=engine.request('capture',name=name)
                engine.step(2)
                picture=Image.open(base/result['path']).convert('RGB')
                picture.save(args.output/(name+'.png'))
                return picture
            first_frame=entity('signal_crate')['frame']
            first=capture('composed-model')
            engine.step(10)
            second=capture('composed-animation')
            assert entity('signal_crate')['frame']!=first_frame
            assert ImageChops.difference(first,second).getbbox(), 'cooked model animation must change visible pixels'
            engine.step(250)  # Respect the existing five-second team-switch cooldown.
            execute('team free')
            execute('give health')
            execute('setviewpos -128 128 24 0')
            engine.step(3)
            assert engine.request('state')['player']['health']==93, ('trigger damage must apply once',engine.request('state'),entity('touch_zone'))
            assert entity('medical_boost')['linked'], 'touch hook must activate the existing target pickup'
            execute('setviewpos -160 0 24 0')
            engine.step(20)
            controls=dict(forward=1,right=0,up=0,yaw=0,pitch=0,fire=False,ads=False,reload=False,melee=False,offhand=False)
            engine.request('input',**controls)
            engine.step(30)
            position=engine.request('state')['player']['origin']
            assert -35<position[0]<-30, ('solid component must stop movement',position)
            controls.update(forward=0,fire=True)
            engine.request('input',**controls)
            engine.step(30)
            controls['fire']=False
            engine.request('input',**controls)
            engine.step(3)
            assert entity('signal_crate') is None, 'real weapon damage must destroy the authored damageable model'
            print('PASS: cooked model animation, solid collision, trigger damage/target hooks, sound emission and destructible health')
        finally:
            shutil.copyfile(engine.log_path,args.output/'components.log')
