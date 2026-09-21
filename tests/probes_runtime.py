#!/usr/bin/env python3
"""Bake an owned scene and measure its reflection on a dynamic PBR model."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from PIL import Image
from cook import cook
from materials_runtime import source as sphere_source
from run import ROOT, content_settings

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary',required=True,type=Path)
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--content',choices=('quake3','openarena'),default='quake3')
parser.add_argument('--output',type=Path,default=Path('/tmp/aftershock-reflection-runtime'))
args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
icds=list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
paks=sorted(args.data.resolve().glob('*.pk3'))
if not paks or len(icds)!=1:
    parser.error('installed content and one lavapipe ICD are required')
with tempfile.TemporaryDirectory(prefix='aftershock-reflection-') as temporary:
    home=Path(temporary);base=home/('baseoa' if args.content=='openarena' else 'baseq3')
    subprocess.run([sys.executable,'tools/level','tests/assets/levels/two_lane.json','--output',str(base)],cwd=ROOT,check=True)
    inputs=home/'source';inputs.mkdir()
    project,document=sphere_source(inputs)
    material=document['materials'][0]['pbrMetallicRoughness']
    material.update(metallicFactor=1,roughnessFactor=.15)
    (inputs/'sphere.gltf').write_text(json.dumps(document))
    cook(project,base)
    description=inputs/'probes.json'
    description.write_text(json.dumps({'map':'two_lane','probes':[{'origin':[0,0,96],'radius':512}]}))
    subprocess.run([sys.executable,'tools/level/probes.py',str(description),'--client',str(args.binary.resolve()),
        '--base',str(base),'--data',str(args.data.resolve()),'--content',args.content,'--output',str(base)],cwd=ROOT,check=True)
    probe=base/'maps/two_lane.asprobe';assert probe.is_file()
    shutil.copyfile(probe,args.output/probe.name)
    for pak in paks:(base/pak.name).symlink_to(pak)
    reflected = {}
    for label,roughness in (('smooth',.15),('rough',1)):
        material['roughnessFactor'] = roughness
        (inputs/'sphere.gltf').write_text(json.dumps(document))
        cook(project,base)
        for old in (base/'screenshots').glob('*.tga'):old.unlink()
        commands=['set g_synchronousClients 1','set fixedtime 20','devmap two_lane','wait 40','team spectator','wait 10',
            'cmd dev_view -160 0 96 10 0 0','wait 10','testmodel models/sphere.iqm','wait 10']
        for name,value in (('off',0),('on',1),('restored',0)):
            commands += [f'set r_reflectionProbes {value}','wait 4',f'screenshot {name}','wait 3']
        commands += ['set r_reflectionProbes 1','vid_restart','wait 40',
            'cmd dev_view -160 0 96 10 0 0','wait 10','testmodel models/sphere.iqm','wait 10',
            'screenshot restarted','wait 3','quit'];(base/'reflection.cfg').write_text('\n'.join(commands)+'\n')
        log=args.output/('client-'+label+'.log')
        env=dict(os.environ,LP_NUM_THREADS='1',VK_DRIVER_FILES=str(icds[0]),VK_ICD_FILENAMES=str(icds[0]))
        with log.open('w') as stream:
            subprocess.run(['timeout','90','xvfb-run','-a','faketime','-f','@2026-01-01 00:00:00 i0.01',str(args.binary.resolve()),
                '+set','fs_basepath',str(home),'+set','fs_homepath',str(home),*content_settings(args.content),
                '+set','net_enabled','0','+set','sv_pure','0','+set','r_mode','3','+set','r_fullscreen','0',
                '+set','r_gamma','1','+set','r_intensity','1','+set','r_overBrightBits','0',
                '+set','con_notifytime','0','+set','cg_draw2D','0','+set','cg_drawGun','0',
                '+set','s_initsound','0','+set','com_maxfps','0','+set','cl_autoRecordDemo','0',
                '+set','dev_tools','0','+exec','reflection.cfg'],cwd=ROOT,env=env,stdout=stream,stderr=subprocess.STDOUT,check=True)
        text=log.read_text()
        assert not any(word in text for word in ('ERROR:','Signal caught','Unknown command',"Can't register model"))
        captures={}
        for name in ('off','on','restored','restarted'):
            path=base/'screenshots'/(name+'.tga');shutil.copyfile(path,args.output/(label+'-'+path.name))
            with Image.open(path) as image:captures[name]=image.convert('RGB').crop((180,100,460,380)).tobytes()
        assert captures['on']==captures['restarted'], 'renderer restart changed reflected image'
        assert captures['off']==captures['restored'], 'reflection toggle changed the baseline'
        changed=sum(a!=b for a,b in zip(captures['off'],captures['on']))
        assert changed>1000, ('baked reflection did not affect the PBR model',changed)
        assert 'Reflection probes: 1' in text
        reflected[label] = [b-a for a,b in zip(captures['off'],captures['on'])]
        print(f'PASS: {label}, baked environment changes {changed} dynamic-model channel bytes and restores/restarts exactly')
    assert sum(a!=b for a,b in zip(reflected['smooth'],reflected['rough'])) > 1000, 'roughness did not change probe contribution'
