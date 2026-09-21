#!/usr/bin/env python3
"""Fire authored material-hit effects through the native weapon and renderer boundary."""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from cook import cook
from run import ROOT,SCRATCH
sys.path.insert(0,str(ROOT))
from tools.agent import Engine

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary',type=Path,required=True)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-weapon-effects-runtime')
args=parser.parse_args()
args.output=args.output.resolve()
args.output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-weapon-effects-') as temporary:
    root=Path(temporary)
    with Engine(args.binary,args.data,args.content,home=root/'home',arguments=['+set','cg_weaponTrace','1','+set','g_weapons','weapons/range_rifle.asweapon']) as engine:
        source=root/'source'
        shutil.copytree(ROOT/'tests/assets/levels',source)
        subprocess.run([sys.executable,'tools/level',str(source/'two_lane.json'),'--output',str(root/'compiled')],cwd=ROOT,check=True)
        shutil.copytree(root/'compiled',engine.base,dirs_exist_ok=True)
        cook(ROOT/'tests/assets/range.json',engine.base)
        definition=json.loads((ROOT/'tests/assets/weapons/rifle.weapon.json').read_text())
        recipes=[dict(name='weapons/range_rifle',kind='weapon',source='weapon.json')]
        for material in definition['materials']:
            path=material['effect']
            effect=dict(version=1,name=path.split('/')[-1],emitters=[dict(name='spark',kind='sprite',material=path,
                        capacity=16,rate=0,burst=8,lifetime_ms=1000,size=8,velocity=[20,0,0],velocity_spread=[10,20,20])])
            filename=path.split('/')[-1]+'.json'
            (source/filename).write_text(json.dumps(effect))
            recipes.append(dict(name=path,kind='effect',source=filename))
            material['effect']+='.asfx'
        (source/'weapon.json').write_text(json.dumps(definition))
        project=source/'assets.json'
        project.write_text(json.dumps(dict(version=1,assets=recipes)))
        cook(project,engine.base)
        engine.request('session',dt=20,seed=161)
        engine.request('map',name='two_lane')
        engine.step(150)
        engine.request('usercmd',forwardmove=0,rightmove=0,upmove=0,angles=[45,0,0],buttons=1,weapon=1)
        engine.step(80)
        engine.request('usercmd',forwardmove=0,rightmove=0,upmove=0,angles=[45,0,0],buttons=0,weapon=1)
        engine.step(2)
        stats=engine.request('effects')
        shutil.copyfile(engine.log_path,args.output/'engine.log')
        assert stats['registered']>=len(definition['materials']) and stats['draws']>0,stats
        assert 'Weapon impact: material=effects/range_default.asfx' in engine.log_path.read_text()
        capture=engine.request('capture',name='weapon-effects')
        engine.step(2)
        shutil.copyfile(engine.base/capture['path'],args.output/'impact.png')
        (args.output/'stats.json').write_text(json.dumps(stats,indent=2)+'\n')
print('PASS: authored weapon hits dispatch visible cooked effects through the native renderer imports')
