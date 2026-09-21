#!/usr/bin/env python3
"""Fire authored material-hit effects through the native weapon and renderer boundary."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from PIL import Image,ImageChops,ImageStat
from cook import cook
from run import ROOT,SCRATCH
sys.path.insert(0,str(ROOT))
from tools.agent import Engine

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--decals',action='store_true')
parser.add_argument('--binary',type=Path,required=True)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-weapon-effects-runtime')
args=parser.parse_args()
args.output=args.output.resolve()
args.output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-weapon-effects-') as temporary:
    root=Path(temporary)
    with Engine(args.binary,args.data,args.content,home=root/'home',arguments=['+set','r_fbo','1','+set','r_decals',str(int(args.decals)),'+set','cg_weaponTrace','1','+set','g_weapons','weapons/range_rifle.asweapon','+set','cg_draw2D','0','+set','cg_drawGun','0','+set','con_notifytime','0']) as engine:
        source=root/'source'
        shutil.copytree(ROOT/'tests/assets/levels',source)
        subprocess.run([sys.executable,'tools/level',str(source/'two_lane.json'),'--output',str(root/'compiled')],cwd=ROOT,check=True)
        shutil.copytree(root/'compiled',engine.base,dirs_exist_ok=True)
        cook(ROOT/'tests/assets/range.json',engine.base)
        definition=json.loads((ROOT/'tests/assets/weapons/rifle.weapon.json').read_text())
        recipes=[dict(name='weapons/range_rifle',kind='weapon',source='weapon.json')]
        if args.decals:
            Image.new('RGBA',(32,32),(170,75,30,240)).save(source/'mark.png')
            Image.new('RGB',(32,32),(128,128,255)).save(source/'normal.png')
            recipes.extend([dict(name='textures/hit_mark',kind='texture',source='mark.png'),
                            dict(name='textures/hit_normal',kind='texture',source='normal.png',format='bc5')])
        for material in definition['materials']:
            path=material['effect']
            effect=dict(version=1,name=path.split('/')[-1],emitters=[dict(name='spark',kind='sprite',material=path,
                        capacity=16,rate=0,burst=8,lifetime_ms=1000,size=8,velocity=[20,0,0],velocity_spread=[10,20,20])])
            if args.decals:
                mark=path+'_mark'
                decal=dict(version=1,name=mark.split('/')[-1],color_map='textures/hit_mark.ktx2',normal_map='textures/hit_normal.ktx2',
                           size=[12,12,8],lifetime_ms=10000,fade_ms=1000,color=[1,1,1,1],normal_strength=1)
                decal_file=mark.split('/')[-1]+'.json'
                (source/decal_file).write_text(json.dumps(decal))
                recipes.append(dict(name=mark,kind='decal',source=decal_file))
                effect['decal']=mark+'.asdc'
            filename=path.split('/')[-1]+'.json'
            (source/filename).write_text(json.dumps(effect))
            recipes.append(dict(name=path,kind='effect',source=filename))
            material['effect']+='.asfx'
        (source/'weapon.json').write_text(json.dumps(definition))
        project=source/'assets.json'
        project.write_text(json.dumps(dict(version=1,assets=recipes)))
        cook(project,engine.base)
        engine.request('session',dt=20,seed=161)
        engine.request('map',name='two_lane')
        engine.step(150)
        engine.request('input',forward=0,right=0,up=0,pitch=45,yaw=0,fire=True)
        engine.step(80)
        engine.request('input',forward=0,right=0,up=0,pitch=45,yaw=0,fire=False)
        engine.step(2)
        stats=engine.request('effects')
        shutil.copyfile(engine.log_path,args.output/'engine.log')
        assert stats['registered']>=len(definition['materials']) and stats['draws']>0,stats
        assert 'Weapon impact: material=effects/range_default.asfx' in engine.log_path.read_text()
        player=engine.request('state')['player']['origin']
        entities=[]
        offset=0
        while offset is not None:
            page=engine.request('entity.list',offset=offset,limit=32)
            entities.extend(page['entities'])
            offset=page['next']
        (args.output/'entities.json').write_text(json.dumps(entities,indent=2)+'\n')
        hits=[row['origin'] for row in entities if row['classname']=='weapon_effect' and row['origin'][2]<player[2]-12]
        assert hits,'no floor-hit event remains for the capture'
        target=[sum(point[axis] for point in hits)/len(hits) for axis in range(3)]
        engine.request('cvar.set',name='cg_fov',value='50')
        # Use the actual hit events, excluding the animated local-player model.
        engine.request('camera',mode='pose',origin=[target[0],target[1],target[2]+80],angles=[90,0,0])
        def capture(name):
            frame=engine.request('capture',name=name)
            engine.step(2)
            path=engine.base/frame['path']
            shutil.copyfile(path,args.output/(name+'.png'))
            with Image.open(path) as image:
                return image.convert('RGB')
        active=capture('impact')
        if args.decals:
            marks=engine.request('decals')
            assert marks['active']>0 and marks['draws']>0 and marks['registered']==len(definition['materials']),marks
            assert marks['dropped']==0,marks
        engine.step(70)
        expired=capture('expired')
        assert engine.request('effects')['particles']==0
        assert sum(ImageStat.Stat(ImageChops.difference(active,expired)).sum)>10000,'material-hit particles did not visibly change the native frame'
        if args.decals:
            assert engine.request('decals')['active']>0,'marks expired with particles'
            engine.request('decals.clear');engine.step(2)
            clean=capture('cleared')
            assert sum(ImageStat.Stat(ImageChops.difference(expired,clean)).sum)>10000,'material-hit decals did not visibly project onto the floor'
        (args.output/'stats.json').write_text(json.dumps(stats,indent=2)+'\n')
print('PASS: authored weapon hits dispatch visible cooked effects through the native renderer imports')
