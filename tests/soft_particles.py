#!/usr/bin/env python3
"""Require actual depth fading near the floor, with multisample and SSAO controls."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from PIL import Image, ImageChops, ImageStat
from cook import cook
from run import ROOT, SCRATCH
sys.path.insert(0,str(ROOT))
from tools.agent import Engine

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary',type=Path,required=True)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--samples',type=int,nargs='+',default=[0,4])
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-soft-particles')
args=parser.parse_args()
args.output=args.output.resolve()
args.output.mkdir(parents=True,exist_ok=True)
for samples in args.samples:
    output=args.output/str(samples)
    output.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='aftershock-soft-particles-') as temporary:
        root=Path(temporary)
        source=root/'source'
        shutil.copytree(ROOT/'tests/assets/levels',source)
        subprocess.run([sys.executable,'tools/level',str(source/'two_lane.json'),'--output',str(root/'compiled')],cwd=ROOT,check=True)
        settings=['+set','r_fbo','1','+set','r_softParticles','1','+set','r_ext_multisample',str(samples),
                  '+set','r_ssao','1' if samples else '0','+set','cg_draw2D','0','+set','cg_drawGun','0','+set','con_notifytime','0']
        with Engine(args.binary,args.data,args.content,home=root/'home',arguments=settings) as engine:
            shutil.copytree(root/'compiled',engine.base,dirs_exist_ok=True)
            (engine.base/'scripts/soft.shader').write_text('effects/soft\n{\n{\nmap $whiteimage\nblendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\nrgbGen vertex\nalphaGen vertex\n}\n}\n')
            definition=dict(version=1,name='soft',emitters=[dict(name='puff',kind='sprite',material='effects/soft',capacity=1,
                            rate=0,burst=1,lifetime_ms=1000,size=16,color=[0,1,1,1],soft=True)])
            (source/'soft.json').write_text(json.dumps(definition))
            project=source/'assets.json'
            project.write_text(json.dumps(dict(version=1,assets=[dict(name='effects/soft',kind='effect',source='soft.json')])))
            cook(project,engine.base)
            engine.request('session',dt=20,seed=161)
            engine.request('map',name='two_lane')
            engine.step(150)
            engine.request('camera',mode='pose',origin=[-100,-160,100],angles=[90,0,0])
            engine.step(4)
            def capture(name):
                frame=engine.request('capture',name=name)
                engine.step(2)
                path=engine.base/frame['path']
                shutil.copyfile(path,output/(name+'.png'))
                with Image.open(path) as image:
                    return image.convert('RGB')
            baseline=capture('baseline')
            asset=engine.request('effects.load',path='effects/soft.asfx')['handle']
            deltas={}
            for name,height in [('far',32),('near',1),('behind',-4)]:
                engine.request('effects.start',asset=asset,origin=[-100,-160,height],angles=[0,0,0],seed=161)
                engine.step(1)
                active=capture(name)
                diff=ImageChops.difference(baseline,active)
                center=diff.crop((310,230,330,250))
                deltas[name]=sum(ImageStat.Stat(center).sum)
                engine.step(60)
            shutil.copyfile(engine.log_path,output/'engine.log')
            (output/'report.json').write_text(json.dumps(deltas,indent=2)+'\n')
            assert deltas['far']>50000,deltas
            assert 0<deltas['near']<deltas['far']*.2,('depth fade is absent',deltas)
            assert deltas['behind']==0,('occluded particle leaked',deltas)
print('PASS: native floor intersections fade with sampled depth, MSAA and SSAO')
