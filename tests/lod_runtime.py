#!/usr/bin/env python3
"""Exercise native mesh LOD selection, cooked reloads and renderer restart."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from PIL import Image,ImageChops,ImageStat
from cook import cook
from lod import grid_source
from run import ROOT,SCRATCH
sys.path.insert(0,str(ROOT))
from tools.agent import Engine

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary',type=Path,required=True)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-lod-runtime')
args=parser.parse_args()
args.output=args.output.resolve()
args.output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-lod-live-') as temporary:
    root=Path(temporary)
    home=root/'home'
    with Engine(args.binary,args.data,args.content,home=home,arguments=['+set','dev_reloadAssets','1',
            '+set','cg_draw2D','0','+set','cg_drawGun','0']) as engine:
        required={'effects','effects.load','effects.start','effects.stop'}
        assert required<=set(engine.request('hello')['commands']),'native effect controls are absent'
        source=root/'source'
        shutil.copytree(ROOT/'tests/assets/levels',source)
        subprocess.run([sys.executable,'tools/level',str(source/'two_lane.json'),'--output',str(root/'compiled')],cwd=ROOT,check=True)
        shutil.copytree(root/'compiled',engine.base,dirs_exist_ok=True)
        (engine.base/'scripts/effects.shader').write_text('effects/test\n{\ncull disable\n{\nmap $whiteimage\nblendFunc GL_SRC_ALPHA GL_ONE\nrgbGen vertex\nalphaGen vertex\n}\n}\n')
        definition=dict(version=1,name='test',emitters=[dict(name='spark',kind='sprite',material='effects/test',
                        capacity=16,rate=0,burst=8,lifetime_ms=1000,size=16,color=[1,.2,.05,1])])
        grid=source/'grid'
        grid.mkdir()
        grid_project,grid_recipe=grid_source(grid)
        grid_recipe['lod_ratios']=[.5,.25]
        def publish_grid():
            grid_project.write_text(json.dumps(dict(version=1,assets=[grid_recipe])))
            cook(grid_project,engine.base)
        publish_grid()
        definition['emitters'][0].update(kind='mesh',model='models/grid.iqm',burst=1,lifetime_ms=60000,size=16)
        effect=source/'test.effect.json'
        effect.write_text(json.dumps(definition))
        project=source/'effects.json'
        project.write_text(json.dumps(dict(version=1,assets=[dict(name='effects/test',kind='effect',source=effect.name)])))
        cook(project,engine.base)
        engine.request('session',dt=20,seed=161)
        engine.request('map',name='two_lane')
        engine.step(150)
        engine.request('camera',mode='pose',origin=[-160,0,80],angles=[0,0,0])
        engine.step(4)
        def capture(name):
            record=engine.request('capture',name=name)
            engine.step(2)
            path=engine.base/record['path']
            shutil.copyfile(path,args.output/(name+'.png'))
            with Image.open(path) as image:
                return image.convert('RGB')
        before=capture('baseline')
        asset=engine.request('effects.load',path='effects/test.asfx')['handle']
        def model():
            return next(row for row in engine.request('assets',kind='models',filter='models/grid.iqm')['items'] if row['name']=='models/grid.iqm')
        assert model()['lods']==3,model()
        handle=engine.request('effects.start',asset=asset,origin=[0,0,80],angles=[0,90,0],seed=161)['handle']
        assert handle
        engine.step(5)
        active=capture('near')
        assert sum(ImageStat.Stat(ImageChops.difference(before,active)).sum)>10000
        assert model()['lodDraws'][0]>0,model()
        memory=engine.request('profile')['memory']
        engine.request('camera',mode='pose',origin=[-900,0,80],angles=[0,0,0])
        engine.step(5)
        capture('far')
        assert model()['lodDraws'][2]>0,model()
        engine.request('camera',mode='pose',origin=[-160,0,80],angles=[0,0,0])
        engine.step(5)
        after=engine.request('profile')['memory']
        assert memory['hunkPermanent']==after['hunkPermanent'] and memory['tags']==after['tags']
        # Removing authored LODs must disable stale sibling files left on disk.
        reloads=model()['reloads']
        del grid_recipe['lod_ratios']
        publish_grid()
        for _ in range(100):
            engine.step(5)
            if model()['reloads']>reloads:
                break
        assert model()['reloads']>reloads and model()['lods']==1,model()
        grid_recipe['lod_ratios']=[.5,.25]
        publish_grid()
        for _ in range(100):
            engine.step(5)
            if model()['lods']==3:
                break
        assert model()['lods']==3,model()
        engine.request('exec',command='vid_restart')
        engine.step(20)
        asset=engine.request('effects.load',path='effects/test.asfx')['handle']
        engine.request('effects.start',asset=asset,origin=[0,0,80],angles=[0,90,0],seed=161)
        engine.step(5)
        assert model()['lods']==3,model()
        capture('restarted')
        shutil.copyfile(engine.log_path,args.output/'engine.log')
print('PASS: visible native LODs, near/far selection, stable memory, stale-file removal, reload and restart')
