#!/usr/bin/env python3
"""Exercise authored filmic exposure/LUT/lens controls on the native renderer."""
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
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-post-runtime')
args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-post-runtime-') as temporary:
    root=Path(temporary);source=root/'source'
    shutil.copytree(ROOT/'tests/assets/levels',source)
    subprocess.run([sys.executable,'tools/level',str(source/'two_lane.json'),'--output',str(root/'compiled')],cwd=ROOT,check=True)
    # A constant color LUT proves the actual grading lookup; no source regeneration.
    Image.new('RGB',(256,16),(190,65,25)).save(source/'grade.png')
    definition=dict(version=1,name='test',exposure_ev=0,lut='textures/grade.ktx2',lut_strength=0)
    path=source/'post.json';path.write_text(json.dumps(definition))
    project=source/'assets.json';project.write_text(json.dumps(dict(version=1,assets=[
        dict(name='textures/grade',kind='texture',source='grade.png'),dict(name='post/test',kind='post',source='post.json')])))
    arguments=['+set','r_fbo','1','+set','r_hdr','2','+set','r_postProcess','1','+set','r_postProfile','post/test.aspost',
               '+set','r_ext_multisample',str(args.samples),'+set','dev_reloadAssets','1',
               '+set','cg_draw2D','0','+set','cg_drawGun','0','+set','con_notifytime','0']
    with Engine(args.binary,args.data,args.content,home=root/'home',arguments=arguments) as engine:
        shutil.copytree(root/'compiled',engine.base,dirs_exist_ok=True);cook(project,engine.base)
        engine.request('session',dt=20,seed=161);engine.request('map',name='two_lane');engine.step(150)
        engine.request('camera',mode='pose',origin=[-100,-160,100],angles=[90,0,0]);engine.step(4)
        def capture(name):
            capture=engine.request('capture',name=name);engine.step(2)
            shutil.copyfile(engine.log_path,args.output/'engine.log')
            path=engine.base/capture['path'];shutil.copyfile(path,args.output/(name+'.png'))
            with Image.open(path) as image:return image.convert('RGB')
        def difference(a,b):return sum(ImageStat.Stat(ImageChops.difference(a,b)).sum)
        def edit(**values):
            definition.update(values);path.write_text(json.dumps(definition));cook(project,engine.base);engine.step(60)
        baseline=capture('baseline')
        edit(exposure_ev=2);bright=capture('bright')
        assert sum(ImageStat.Stat(bright).mean)>sum(ImageStat.Stat(baseline).mean)*1.15,'authored exposure did not brighten the scene'
        edit(exposure_ev=0,lut_strength=1);graded=capture('graded')
        pixel=graded.getpixel((320,200));assert pixel[0]>pixel[1]*2 and pixel[1]>pixel[2]*1.5,pixel
        edit(lut_strength=0,vignette=1);vignette=capture('vignette');assert difference(baseline,vignette)>10000
        edit(vignette=0,sharpen=1);sharp=capture('sharp');assert difference(baseline,sharp)>1000
        edit(sharpen=0,focus_distance=1000,focus_range=1,dof_radius=8);blur=capture('blur');assert difference(baseline,blur)>10000
        edit(dof_radius=0,grain=.5);grain=capture('grain');engine.step(1);assert difference(grain,capture('grain-next'))>10000
        edit(grain=0);assert difference(baseline,capture('restored'))==0
        engine.request('exec',command='vid_restart');engine.step(60)
        assert difference(baseline,capture('restart'))==0
print('PASS: native post exposure, LUT, sharpening, vignette, grain, depth of field, reload and restart')
