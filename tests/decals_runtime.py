#!/usr/bin/env python3
"""Project a volume onto the actual scene depth; check normal maps, fade and reload."""
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
parser.add_argument('--binary',type=Path,required=True)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--samples',type=int,default=0)
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-decals-runtime')
args=parser.parse_args()
args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-decals-') as temporary:
    root=Path(temporary);source=root/'source'
    shutil.copytree(ROOT/'tests/assets/levels',source)
    subprocess.run([sys.executable,'tools/level',str(source/'two_lane.json'),'--output',str(root/'compiled')],cwd=ROOT,check=True)
    Image.new('RGBA',(32,32),(160,85,35,230)).save(source/'color.png')
    Image.new('RGB',(32,32),(128,128,255)).save(source/'normal.png')
    definition=dict(version=1,name='bullet',color_map='textures/decal_color.ktx2',normal_map='textures/decal_normal.ktx2',
                    size=[64,64,8],lifetime_ms=2000,fade_ms=1000,color=[1,1,1,1],normal_strength=1)
    decal=source/'bullet.json';decal.write_text(json.dumps(definition))
    project=source/'assets.json'
    project.write_text(json.dumps(dict(version=1,assets=[dict(name='textures/decal_color',kind='texture',source='color.png'),
        dict(name='textures/decal_normal',kind='texture',source='normal.png',format='bc5'),
        dict(name='decals/bullet',kind='decal',source='bullet.json')])))
    settings=['+set','r_fbo','1','+set','r_decals','1','+set','r_ext_multisample',str(args.samples),
              '+set','cg_draw2D','0','+set','cg_drawGun','0','+set','con_notifytime','0']
    with Engine(args.binary,args.data,args.content,home=root/'home',arguments=settings) as engine:
        shutil.copytree(root/'compiled',engine.base,dirs_exist_ok=True)
        cook(project,engine.base)
        engine.request('session',dt=20,seed=161)
        engine.request('map',name='two_lane');engine.step(150)
        engine.request('camera',mode='pose',origin=[-100,-160,100],angles=[90,0,0]);engine.step(4)
        def capture(name):
            frame=engine.request('capture',name=name);engine.step(2)
            path=engine.base/frame['path'];shutil.copyfile(path,args.output/(name+'.png'))
            with Image.open(path) as image:return image.convert('RGB')
        def difference(a,b):return sum(ImageStat.Stat(ImageChops.difference(a,b)).sum)
        baseline=capture('baseline')
        asset=engine.request('decals.load',path='decals/bullet.asdc')['handle']
        def project_at(height):
            return engine.request('decals.project',asset=asset,origin=[-100,-160,height],angles=[0,0,0])['handle']
        assert project_at(0)
        engine.step(1);active=capture('active');stats=engine.request('decals')
        assert stats['active']==1 and stats['draws']>0 and stats['dropped']==0,stats
        assert difference(baseline,active)>100000,'projected decal is absent'
        engine.step(70);faded=capture('faded')
        assert 0<difference(baseline,faded)<difference(baseline,active)*.8
        engine.step(40);assert engine.request('decals')['active']==0
        assert difference(baseline,capture('expired'))==0
        # A projected volume floating above the floor must not become a billboard.
        assert project_at(32);engine.step(1)
        assert difference(baseline,capture('outside'))==0
        engine.request('decals.clear')
        Image.new('RGB',(32,32),(248,128,150)).save(source/'normal.png')
        cook(project,engine.base);engine.step(60)
        assert project_at(0);engine.step(1)
        tilted=capture('tilted')
        assert difference(active,tilted)>1000,'normal map did not affect projected lighting'
        engine.request('decals.clear')
        definition['color']=[.3,1,.3,1];decal.write_text(json.dumps(definition))
        cook(project,engine.base);engine.step(60)
        assert engine.request('decals')['reloads']>0
        assert project_at(0);engine.step(1)
        assert difference(tilted,capture('reloaded'))>1000
        engine.request('exec',command='vid_restart');engine.step(40)
        assert engine.request('decals')['registered']==0
        shutil.copyfile(engine.log_path,args.output/'engine.log')
print('PASS: depth-projected normal-mapped decal, bounded fade, texture/definition reload and restart')
