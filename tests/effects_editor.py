#!/usr/bin/env python3
"""Edit an effect through the shared ImGui/agent actions and replay its cooked change."""
import argparse
import json
from pathlib import Path
import shutil
import sys
import tempfile

from cook import cook
from run import ROOT,SCRATCH,content_maps
sys.path.insert(0,str(ROOT))
from tools.agent import Engine

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary',type=Path,required=True)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-effects-editor')
args=parser.parse_args()
args.output=args.output.resolve()
args.output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-effects-editor-') as temporary:
    with Engine(args.binary,args.data,args.content,home=Path(temporary)/'home',arguments=['+set','dev_reloadAssets','1']) as engine:
        assert 'effects.edit' in engine.request('hello')['commands'],'effect source editor is absent'
        source=engine.base/'effects_source'
        source.mkdir()
        (engine.base/'scripts').mkdir(exist_ok=True)
        (engine.base/'scripts/effects.shader').write_text('effects/test\n{\ncull disable\n{\nmap $whiteimage\nblendFunc GL_SRC_ALPHA GL_ONE\nrgbGen vertex\nalphaGen vertex\n}\n}\n')
        definition=dict(version=1,name='test',emitters=[dict(name='spark',kind='sprite',material='effects/test',
                        capacity=16,rate=0,burst=2,lifetime_ms=1000,size=16)])
        effect=source/'test.json'
        effect.write_text(json.dumps(definition))
        original=effect.read_bytes()
        project=source/'assets.json'
        project.write_text(json.dumps(dict(version=1,assets=[dict(name='effects/test',kind='effect',source=effect.name)])))
        cook(project,engine.base)
        engine.request('session',dt=20,seed=161)
        engine.request('map',name=content_maps(args.content)[0])
        engine.step(150)
        engine.request('panel',name='Effects')
        engine.request('effects.edit',action='source',text='effects_source/test.json')
        engine.step(3)
        state=engine.request('editor.state')
        assert state['panel']=='Effects' and state['effects']['result']=='source_loaded',state
        asset=engine.request('effects.load',path='effects/test.asfx')['handle']
        engine.request('effects.start',asset=asset,origin=[0,0,80],angles=[0,0,0],seed=161)
        assert engine.request('effects')['particles']==2
        engine.step(60)
        definition['emitters'][0]['burst']=5
        edited=json.dumps(definition)
        engine.request('effects.edit',action='text',text=edited)
        assert engine.request('editor.state')['effects']['dirty']
        engine.request('effects.edit',action='save')
        engine.step(2)
        assert engine.request('editor.state')['effects']['result']=='saved'
        assert effect.read_text()==edited and (source/'test.json.bak.000').read_bytes()==original
        reloads=engine.request('effects')['reloads']
        cook(project,engine.base)
        for _ in range(100):
            engine.step(5)
            if engine.request('effects')['reloads']>reloads:
                break
        assert engine.request('effects')['reloads']>reloads
        engine.request('effects.start',asset=asset,origin=[0,0,80],angles=[0,0,0],seed=161)
        assert engine.request('effects')['particles']==5
        # External changes must survive a stale editor save.
        effect.write_text(edited+'\n')
        engine.request('effects.edit',action='text',text=json.dumps(dict(definition,name='changed')))
        engine.request('effects.edit',action='save')
        engine.step(2)
        assert engine.request('editor.state')['effects']['result']=='source_changed'
        assert effect.read_text()==edited+'\n'
        engine.request('effects.edit',action='undo')
        assert not engine.request('editor.state')['effects']['dirty']
        engine.request('effects.edit',action='source',text='other/test.json')
        engine.step(2)
        assert engine.request('editor.state')['effects']['result']=='invalid_path'
        capture=engine.request('capture',name='effects-editor')
        engine.step(2)
        shutil.copyfile(engine.base/capture['path'],args.output/'panel.png')
        shutil.copyfile(engine.log_path,args.output/'engine.log')
print('PASS: ImGui effect source actions, backup, save conflicts and live cooked replay')
