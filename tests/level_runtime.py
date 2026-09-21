#!/usr/bin/env python3
"""Compile the owned level and prove bots traverse each lane in the native engine."""
import argparse
import copy
import json
import os
from pathlib import Path
from run import SCRATCH
import re
import shutil
import struct
import subprocess
import sys
import tempfile

from run import ROOT, content_bots, content_settings

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--server',type=Path,required=True)
parser.add_argument('--client',type=Path,required=True)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=(SCRATCH / 'aftershock-level-runtime'))
args = parser.parse_args()
args.output = args.output.resolve()
args.output.mkdir(parents=True,exist_ok=True)
paks = sorted(args.data.resolve().glob('*.pk3'))
icds = list(Path('/usr/share/vulkan/icd.d').glob('lvp*.json'))
if not paks or len(icds)!=1:
    parser.error('installed content and exactly one lavapipe ICD are required')
fixture = ROOT/'tests/assets/levels'
definition = json.loads((fixture/'two_lane.json').read_text())
env = dict(os.environ,LC_ALL='C',LP_NUM_THREADS='1',VK_DRIVER_FILES=str(icds[0]),VK_ICD_FILENAMES=str(icds[0]))
with tempfile.TemporaryDirectory(prefix='aftershock-level-live-') as temporary:
    home = Path(temporary)
    base = home/('baseoa' if args.content=='openarena' else 'baseq3')
    source = home/'source'
    source.mkdir()
    shutil.copytree(fixture/'assets',source/'assets')
    for variant in ('both','door-stairs','ramp'):
        doc = copy.deepcopy(definition)
        if variant!='both':
            doc['connections'] = [c for c in doc['connections'] if c['at']==(128 if variant=='door-stairs' else -128)]
        (source/'level.json').write_text(json.dumps(doc))
        # Compile before adding installed content: licensed paks are only symlinked for runtime.
        compiled = home/('compiled-'+variant)
        result = subprocess.run([sys.executable,str(ROOT/'tools/level'),str(source/'level.json'),'--output',str(compiled)],
                                capture_output=True,text=True,check=True)
        (args.output/(variant+'-compile.json')).write_text(result.stdout)
        shutil.copyfile(compiled/'compile.log',args.output/(variant+'-compile.log'))
        if base.exists():
            shutil.rmtree(base)
        shutil.copytree(compiled,base)
        for pak in paks:
            (base/pak.name).symlink_to(pak)
        commands = ['set g_synchronousClients 1','set fixedtime 20','set sv_fps 50','set g_logSync 1',
                    'devmap two_lane',*[f'addbot {bot} 4' for bot in content_bots(args.content)],'wait 6000','quit']
        (base/'level-bots.cfg').write_text('\n'.join(commands)+'\n')
        log = args.output/(variant+'-bots.log')
        with log.open('w') as stream:
            subprocess.run(['timeout','90','faketime','-f','@2026-01-01 00:00:00 i0.01',str(args.server.resolve()),
                            '+set','fs_basepath',str(home),'+set','fs_homepath',str(home),*content_settings(args.content),
                            '+set','dedicated','1','+set','net_enabled','0','+set','sv_pure','0','+exec','level-bots.cfg'],
                           cwd=ROOT,env=env,stdout=stream,stderr=subprocess.STDOUT,check=True)
        text = log.read_text()
        assert 'Static game loaded.' in text and 'ClientBegin: 1' in text
        assert not any(e in text for e in ('ERROR:','Fatal','AAS not initialized','Signal caught'))
        # The only shotgun is in the middle room; all spawn points are in the end rooms.
        # Isolating each lane proves the door/stairs and ramp both support actual AAS travel.
        for bot in (0,1):
            assert f'Item: {bot} weapon_shotgun' in text, (variant,bot,'never reached middle room')
        assert re.search(r'Item: [01] ammo_shells',text), (variant,'no east-room pickup')
        assert len(re.findall(r'^Kill:',text,re.M))>=2, (variant,'bots did not engage')
        print(f'PASS: {args.content} {variant}; both bots reached the middle and fought; east pickup reached')
        if variant=='both':
            commands = ['set g_synchronousClients 1','set fixedtime 20','devmap two_lane','wait 60']
            for name,origin,angle in [('west','-160 0 80',0),('middle','700 0 80',0),('east','1696 0 112',180)]:
                commands += [f'setviewpos {origin} {angle}','wait 20',f'screenshot level-{name}','wait 2']
            (base/'level-view.cfg').write_text('\n'.join(commands+['quit'])+'\n')
            log = args.output/'client.log'
            with log.open('w') as stream:
                subprocess.run(['timeout','90','xvfb-run','-a','faketime','-f','@2026-01-01 00:00:00 i0.01',str(args.client.resolve()),
                                '+set','fs_basepath',str(home),'+set','fs_homepath',str(home),*content_settings(args.content),
                                '+set','net_enabled','0','+set','sv_pure','0','+set','r_mode','3','+set','r_fullscreen','0',
                                '+set','s_initsound','0','+set','com_maxfps','0','+exec','level-view.cfg'],
                               cwd=ROOT,env=env,stdout=stream,stderr=subprocess.STDOUT,check=True)
            text = log.read_text()
            assert 'VK_RENDERER:' in text and 'llvmpipe' in text and 'Static cgame loaded.' in text
            assert not any(e in text for e in ('ERROR:','Signal caught','Shader not found','could not find shader'))
            images = []
            for name in ('west','middle','east'):
                path = base/'screenshots'/f'level-{name}.tga'
                data = path.read_bytes()
                assert struct.unpack_from('<HH',data,12)==(640,480)
                images.append(data)
                shutil.copyfile(path,args.output/path.name)
            assert len(set(images))==3, 'fixed room views did not change'
print('PASS: owned level rendered from all three rooms; three room views collected for review')
