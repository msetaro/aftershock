#!/usr/bin/env python3
"""Exercise authored effects through the native agent, rendering and reload paths."""
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
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-effects-runtime')
args=parser.parse_args()
args.output=args.output.resolve()
args.output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-effects-live-') as temporary:
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
        assert asset
        memory=engine.request('profile')['memory']
        handle=engine.request('effects.start',asset=asset,origin=[0,0,80],angles=[0,0,0],seed=161)['handle']
        assert handle and engine.request('effects')['particles']==8
        engine.step(5)
        active=capture('active')
        stats=engine.request('effects')
        assert stats['particles']==8 and stats['draws']>=8,stats
        difference=ImageChops.difference(before,active)
        assert sum(ImageStat.Stat(difference).sum)>10000,'effect did not change the native frame'
        engine.step(60)
        assert engine.request('effects')['particles']==0
        capture('expired')
        after=engine.request('profile')['memory']
        assert memory['hunkPermanent']==after['hunkPermanent']
        assert memory['tags']==after['tags'],'effect playback allocated persistent engine memory'
        # Publish an edit; the same registered asset must pick it up automatically.
        reloads=engine.request('effects')['reloads']
        definition['emitters'][0]['burst']=3
        effect.write_text(json.dumps(definition))
        cook(project,engine.base)
        for _ in range(100):
            engine.step(5)
            if engine.request('effects')['reloads']>reloads:
                break
        assert engine.request('effects')['reloads']>reloads
        engine.request('effects.start',asset=asset,origin=[0,0,80],angles=[0,0,0],seed=161)
        assert engine.request('effects')['particles']==3
        engine.request('exec',command='vid_restart')
        engine.step(20)
        assert engine.request('effects')['particles']==0
        asset=engine.request('effects.load',path='effects/test.asfx')['handle']
        engine.request('effects.start',asset=asset,origin=[0,0,80],angles=[0,0,0],seed=161)
        engine.step(2)
        assert engine.request('effects')['particles']==3
        capture('restarted')
        shutil.copyfile(engine.log_path,args.output/'engine.log')
print('PASS: native effect control, visible sprites, expiry, stable memory, watched reload and renderer restart')
