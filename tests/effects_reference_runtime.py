#!/usr/bin/env python3
"""Capture every original reference effect through the native renderer."""
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
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-effects-reference-runtime')
args=parser.parse_args()
args.output=args.output.resolve()
args.output.mkdir(parents=True,exist_ok=True)
fixture=ROOT/'tests/assets/effects'
with tempfile.TemporaryDirectory(prefix='aftershock-reference-effects-') as temporary:
    root=Path(temporary)
    with Engine(args.binary,args.data,args.content,home=root/'home',arguments=['+set','r_fbo','1','+set','r_decals','1','+set','cg_draw2D','0','+set','cg_drawGun','0','+set','con_notifytime','0']) as engine:
        source=root/'source'
        shutil.copytree(ROOT/'tests/assets/levels',source)
        subprocess.run([sys.executable,'tools/level',str(source/'two_lane.json'),'--output',str(root/'compiled')],cwd=ROOT,check=True)
        shutil.copytree(root/'compiled',engine.base,dirs_exist_ok=True)
        cook(fixture/'assets.json',engine.base)
        cook(ROOT/'tests/assets/decals/assets.json',engine.base)
        shutil.copyfile(fixture/'reference.shader',engine.base/'scripts/reference.shader')
        engine.request('session',dt=20,seed=161)
        engine.request('map',name='two_lane')
        engine.step(150)
        engine.request('camera',mode='pose',origin=[-64,0,80],angles=[0,0,0])
        engine.step(4)
        def capture(name):
            frame=engine.request('capture',name=name)
            engine.step(2)
            path=engine.base/frame['path']
            shutil.copyfile(path,args.output/(name+'.png'))
            with Image.open(path) as image:
                return image.convert('RGB')
        baseline=capture('baseline')
        report={}
        for name in ('muzzle','impact_metal','impact_stone','smoke','sparks','dust','shell','explosion','tracer'):
            asset=engine.request('effects.load',path=f'effects/reference/{name}.asfx')['handle']
            angles=[0,90,0] if name in ('shell','tracer') else [0,0,0]
            origin=[0,-100,80] if name=='tracer' else [0,0,80]
            before=engine.request('effects')
            engine.request('effects.start',asset=asset,origin=origin,angles=angles,seed=161)
            engine.step(1)
            active=capture(name)
            stats=engine.request('effects')
            difference=sum(ImageStat.Stat(ImageChops.difference(baseline,active)).sum)
            report[name]=dict(difference=difference,draws=stats['draws']-before['draws'],lights=stats['lightDraws']-before['lightDraws'])
            (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
            shutil.copyfile(engine.log_path,args.output/'engine.log')
            assert report[name]['draws']>0 and difference>1000,(name,report[name])
            if name in ('muzzle','explosion'):
                assert report[name]['lights']>0
            engine.step(140)
            assert engine.request('effects')['particles']==0
        engine.request('camera',mode='pose',origin=[-100,-160,100],angles=[90,0,0]);engine.step(4)
        baseline=capture('decal-baseline')
        for name in ('bullet','scorch','blood'):
            asset=engine.request('decals.load',path=f'decals/reference/{name}.asdc')['handle']
            engine.request('decals.project',asset=asset,origin=[-100,-160,0],angles=[0,0,0]);engine.step(1)
            active=capture('decal-'+name)
            difference=sum(ImageStat.Stat(ImageChops.difference(baseline,active)).sum)
            stats=engine.request('decals')
            assert stats['active']==1 and stats['draws']>0 and stats['dropped']==0,stats
            assert difference>1000,(name,difference)
            report['decal-'+name]=dict(difference=difference,draws=stats['draws'])
            engine.request('decals.clear');engine.step(2)
            clean=capture('cleared-'+name)
            assert sum(ImageStat.Stat(ImageChops.difference(baseline,clean)).sum)==0
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        shutil.copyfile(engine.log_path,args.output/'engine.log')
print('PASS: all original reference sprite/mesh/trail effects change native frames and expire')
