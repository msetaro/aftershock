#!/usr/bin/env python3
"""Owned sketch semantics and the complete native sketch-to-level command."""
import argparse
import copy
import json
import re
from pathlib import Path
import subprocess
import sys
import tempfile

from PIL import Image
from run import ROOT
sys.path.insert(0,str(ROOT/'tools/level'))
from sketch import measure
from theme import assemble
from polygons import footprint,ruled_openings
from tools.agent.formats import validate as validate_format
from validate import validate
from tools.assets.manifest import validate as validate_manifest

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--library',type=Path,required=True)
parser.add_argument('--modules',type=Path,required=True)
parser.add_argument('--client',type=Path)
parser.add_argument('--server',type=Path)
parser.add_argument('--output',type=Path)
parser.add_argument('--content',choices=['quake3','openarena'],default='quake3')
parser.add_argument('--data',type=Path,default=Path.home()/'.q3a/baseq3')
args=parser.parse_args()
fixture=ROOT/'tests/assets/sketch'
validate_manifest(fixture)
notes=json.loads((fixture/'notes.json').read_text())
with Image.open(fixture/'reference.png') as image:
    interpretation,level,_=measure(image,notes)
assert len(level['spawns'])==2, 'drawn spawn marks did not become player spawns'
assert {i['id'] for i in level['intents']}=={'main_route','plaza_hold'}, 'drawing-only fields leaked into playable intents'
assert level['pickups']==notes['pickups'] and level['viewpoints']==notes['viewpoints']
assert interpretation['notes']['free_text']==notes['free_text'], 'free text/decisions lost'
assert not interpretation['assumptions'], interpretation['assumptions']
assert len([s for s in level['shapes'] if s['kind']=='building'])==3
assert any(s['kind']=='wall' for s in level['shapes']) and len(level['shapes'])==6
validate_format('level',level,fixture/'notes.json')
with tempfile.TemporaryDirectory(prefix='aftershock-sketch-build-') as temporary:
    root=Path(temporary)
    theme=json.loads((ROOT/'tools/level/themes/manhattan.json').read_text())
    first=assemble(level,theme,args.library,args.modules,root/'project',seed=164)
    for building in (s for s in first['shapes'] if s['kind']=='building'):
        assert any(o['sill']==0 and o['height']>=80 for o in ruled_openings(building,footprint(building['shape']),1)), 'enterable building has no entrance'
    _,geometry=validate(first,root/'project/assets')
    assert geometry['reachable_spawns']==2
    assert (root/'project/assembly.json').is_file(), 'default entrance decisions were not recorded'
    changed=copy.deepcopy(notes)
    changed['overrides']['building_7']=dict(floors=2,height=256,opening_rules=[dict(face_point=[0,-154],spacing=1024,width=96,sill=48,height=48,floors=[1])])
    with Image.open(fixture/'reference.png') as image:
        _,edited,_=measure(image,changed,interpretation)
    second=assemble(edited,theme,args.library,args.modules,root/'edited',seed=164)
    before,after=[{s['id']:s for s in v['shapes']} for v in (first,second)]
    assert before.keys()==after.keys() and all((before[k]!=after[k])==(k=='building_7') for k in before)
    assert first['props']==second['props'], 'unrelated dressing changed during storey edit'
    validate(second,root/'edited/assets')
    explicit=copy.deepcopy(level)
    explicit['shapes'][0]['openings']=[]
    closed=assemble(explicit,theme,args.library,args.modules,root/'explicit',seed=164)
    assert closed['shapes'][0]['openings']==[], 'explicit opening override ignored'
    if args.client or args.server:
        assert args.client and args.server and args.output
        command=[sys.executable,'tools/level','build','--sketch',str(fixture/'reference.png'),'--notes',str(fixture/'notes.json'),
                 '--theme','manhattan','--out',str(args.output),'--library',str(args.library),'--modules',str(args.modules),
                 '--client',str(args.client),'--server',str(args.server),'--content',args.content,'--data',str(args.data)]
        result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
        assert result.returncode==0,result.stderr+'\n'+result.stdout
        report=json.loads(result.stdout)
        assert report['status']=='passed' and report['shooter']['passed'] and report['routes']['passed'],report
        assert report['overhead']['passed'] and report['runtime']['bots']['kills']>=2 and not report['runtime']['bots']['stuck']
        assert report['runtime']['views'] and report['runtime']['flythrough']
        for name in ('report.json','trace/overlay.png','overhead/compiled-overhead.png','overhead/overhead-difference.png','project/assets/CREDITS'):
            assert (args.output/name).is_file(),name
        edited_notes=root/'edited-notes.json'
        edited_notes.write_text(json.dumps(changed))
        next_output=args.output.with_name(args.output.name+'-edited')
        follow=command[:]
        follow[1]='tools/agent'
        follow[follow.index('--notes')+1]=str(edited_notes)
        follow[follow.index('--out')+1]=str(next_output)
        follow.extend(['--previous',str(args.output/'trace/interpretation.json')])
        result=subprocess.run(follow,cwd=ROOT,capture_output=True,text=True)
        assert result.returncode==0,result.stderr+'\n'+result.stdout
        assert json.loads(result.stdout)['status']=='passed'
        def groups(path):
            parts=re.split(r'// shape ([a-z0-9_]+)\n',path.read_text())
            result={}
            for identity,text in zip(parts[1::2],parts[2::2]):
                result.setdefault(identity,[]).extend(re.findall(r'\{\s*(?:\([^\n]+\n)+\}',text))
            return result
        before,after=[groups(out/'compiled/maps/sketch_reference.map') for out in (args.output,next_output)]
        assert before.keys()==after.keys() and all((before[k]!=after[k])==(k=='building_7') for k in before)
        repeated=subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
        assert repeated.returncode and 'exists' in repeated.stderr, 'existing evidence was overwritten'
        uncertain=copy.deepcopy(notes)
        next(m for m in uncertain['marks'] if m.get('kind')=='route')['confidence']=.3
        edited_notes.write_text(json.dumps(uncertain))
        ambiguous=command[:]
        ambiguous[ambiguous.index('--notes')+1]=str(edited_notes)
        ambiguous[ambiguous.index('--out')+1]=str(args.output.with_name(args.output.name+'-ambiguous'))
        rejected=subprocess.run(ambiguous,cwd=ROOT,capture_output=True,text=True)
        assert rejected.returncode and json.loads(rejected.stdout)['assumptions'], 'integrated build silently accepted ambiguity'

print('PASS: owned reference semantics, recorded entrances and stable building-7 edit'+('; complete native build/agent iteration' if args.client else ''))
