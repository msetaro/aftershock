#!/usr/bin/env python3
"""Exercise authored menus, controller-key navigation, rebinding and HUD at three resolutions."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time
from PIL import Image, ImageChops
from cook import cook
from run import ROOT, SCRATCH
sys.path.insert(0,str(ROOT))
from tools.agent import Engine

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary',type=Path,required=True)
parser.add_argument('--font',type=Path,default=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'))
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-ui-runtime')
parser.add_argument('--size',action='append',help='WIDTHxHEIGHT; default 1080p,1440p,4K')
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-ui-runtime-') as temporary:
    root=Path(temporary)
    source=root/'source'
    source.mkdir()
    shutil.copyfile(args.font,source/'font.ttf')
    definition=json.loads((ROOT/'tests/assets/ui/shell.json').read_text())
    document=source/'shell.json'
    document.write_text(json.dumps(definition,ensure_ascii=False))
    project=source/'assets.json'
    project.write_text(json.dumps(dict(version=1,assets=[dict(name='ui/shell',kind='ui',source='shell.json')])))
    owned=root/'owned'
    with (args.output/'compile.json').open('w') as log:
        subprocess.run([sys.executable,'tools/level',str(ROOT/'tests/assets/levels/two_lane.json'),'--output',str(owned)],
                       cwd=ROOT,stdout=log,check=True,timeout=600)
    assert not list(owned.glob('*.pk3'))
    for dimensions in args.size or ['1920x1080','2560x1440','3840x2160']:
        width,height=map(int,dimensions.split('x'))
        output=args.output/dimensions
        output.mkdir(exist_ok=True)
        home=root/dimensions
        base=home/('baseoa' if args.content=='openarena' else 'baseq3')
        shutil.copytree(owned,base)
        definition['texts'][0]['values']['en']='AFTERSHOCK'
        document.write_text(json.dumps(definition,ensure_ascii=False))
        cook(project,base)
        settings=['+set','r_mode','-1','+set','r_customwidth',str(width),'+set','r_customheight',str(height),
                  '+set','ui_document','ui/shell.asui','+set','ui_language','en','+set','ui_safeArea','.05',
                  '+set','con_notifytime','0','+set','s_volume','.5']
        with Engine(args.binary,args.data,args.content,home=home,arguments=settings) as engine:
            try:
                engine.request('session',dt=20,seed=17)
                engine.step(4)
                def execute(command):
                    engine.request('exec',command=command)
                    engine.step(2)
                def info():
                    execute('ui_info')
                    rows=re.findall(r'UI document: loaded=1 page=(\w+) focus=(\w+) locale=([\w-]+) binding=(\d+) size=(\d+)x(\d+) draws=(\d+) hud=(\d+)',engine.log_path.read_text(errors='replace'))
                    assert rows, 'authored UI must load and report its live state'
                    row=rows[-1]
                    assert tuple(map(int,row[4:6]))==(width,height) and int(row[6])>0,row
                    return row
                def key(name):
                    engine.request('key',name=name,down=True)
                    engine.step()
                    engine.request('key',name=name,down=False)
                    engine.step()
                def capture(name):
                    result=engine.request('capture',name=name)
                    engine.step(2)
                    image=Image.open(base/result['path']).convert('RGB')
                    assert image.size==(width,height),image.size
                    image.save(output/(name+'.png'))
                    return image
                assert info()[:3]==('main','play','en')
                english=capture('main')
                key('PAD0_DPAD_DOWN')
                assert info()[1]=='options'
                key('PAD0_A')
                assert info()[:2]==('options','volume')
                key('PAD0_DPAD_RIGHT')
                assert abs(float(engine.request('cvar.get',name='s_volume')['value'])-.6)<.001
                key('PAD0_DPAD_DOWN')
                key('PAD0_A')
                assert info()[3]=='1'
                key('PAD0_X')
                assert info()[3]=='0'
                execute('bind PAD0_X')
                assert '"PAD0_X" = "+attack"' in engine.log_path.read_text(), 'real binding must change'
                capture('options')
                key('PAD0_A')
                key('PAD0_B')
                assert info()[3]=='0'
                key('PAD0_B')
                assert info()[0]=='main'
                execute('set ui_language ar')
                assert info()[2]=='ar'
                arabic=capture('arabic')
                assert ImageChops.difference(english,arabic).getbbox(), 'localized glyphs must visibly change'
                execute('set ui_language en; set dev_reloadAssets 1')
                definition['texts'][0]['values']['en']='AFTERSHOCK II'
                document.write_text(json.dumps(definition,ensure_ascii=False))
                cook(project,base)
                execute('ui_reload')
                time.sleep(.2)
                engine.step(8)
                changed=capture('reloaded')
                assert ImageChops.difference(english,changed).getbbox(), 'source edit must reach visible UI'
                key('PAD0_A')
                engine.step(80)
                assert engine.request('state')['player'], 'authored Play action must start the map'
                assert int(info()[7])>0, 'game must draw the authored HUD'
                capture('hud')
                print('PASS:',dimensions,'main/options/HUD, controller keys, rebinding, localization and source reload')
            finally:
                shutil.copyfile(engine.log_path,output/'engine.log')
