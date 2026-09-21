#!/usr/bin/env python3
"""Cook data-authored presentation effects without changing gameplay or old fixtures."""
import argparse
import copy
import hashlib
import json
import os
import shlex
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from cook import cook
from run import ROOT,SCRATCH,run

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-effects')
parser.add_argument('--cc',default='gcc')
parser.add_argument('--cxx',default='g++')
args=parser.parse_args()
args.output=args.output.resolve()
args.output.mkdir(parents=True,exist_ok=True)
os.environ['CXX']=args.cxx
with tempfile.TemporaryDirectory(prefix='aftershock-effect-source-') as temporary:
    source=Path(temporary)
    definition=dict(version=1,name='impact',emitters=[dict(name='sparks',kind='sprite',material='effects/spark',
        capacity=16,rate=0,burst=8,lifetime_ms=1000,size=2,velocity=[100,0,40],gravity=[0,0,-80],drag=.5,
        collision=True,soft=True,lit=False,color=[1,.5,.1,1],flipbook=dict(columns=4,rows=2,fps=16))])
    effect=source/'impact.effect.json'
    effect.write_text(json.dumps(definition))
    project=source/'assets.json'
    project.write_text(json.dumps(dict(version=1,assets=[dict(name='effects/impact',kind='effect',source=effect.name)])))
    result=cook(project,args.output)
    assert result['built']==['effects/impact']
    path=args.output/'effects/impact.asfx'
    data=path.read_bytes()
    magic,version,size,hashed=struct.unpack_from('<8sII32s',data)
    assert magic==b'ASEFFECT' and version==1 and size==len(data)-48
    assert hashlib.sha256(data[48:]).digest()==hashed
    assert data[48:80].split(b'\0',1)[0]==b'impact'
    assert struct.unpack_from('<I',data,80)[0]==1
    assert size==36+244, 'effect payload must match the fixed native record layout'
    name,material,model=struct.unpack_from('<32s64s64s',data,84)
    assert name.split(b'\0',1)[0]==b'sparks' and material.split(b'\0',1)[0]==b'effects/spark' and not model.strip(b'\0')
    assert struct.unpack_from('<7I',data,244)==(0,16,8,1000,3,4,2)
    values=struct.unpack_from('<14f',data,272)
    assert values[:9]==(0,2,100,0,40,0,0,-80,.5) and values[-1]==16
    manifest=json.loads((args.output/'effects/impact.manifest.json').read_text())
    assert {f['path'] for f in manifest['inputs']}=={effect.name}
    index=(args.output/'cook.index').read_bytes()
    assert struct.unpack_from('<I',index,48)[0]==1 and struct.unpack_from('<I',index,152)[0]==8
    assert cook(project,args.output)['built']==[]
    sha_object=args.output/'sha256.o'
    run([*shlex.split(args.cc),'-std=c99','-O2','-c','third_party/sha256/sha-256.c','-o',sha_object])
    probe=args.output/'native-probe'
    run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti','-ffp-contract=off','-fno-fast-math',
         '-Wall','-Wextra','-Werror','-fsanitize=undefined','-fno-sanitize-recover=all',
         'tests/probes/effects.cpp','engine/effects/effects.cpp',sha_object,'-o',probe])
    run([probe,path])

    definition['emitters'][0]['size']=4
    effect.write_text(json.dumps(definition))
    assert cook(project,args.output)['built']==['effects/impact'] and path.read_bytes()!=data
    def rejected(reason):
        result=subprocess.run([sys.executable,'tools/cook',str(project),'--output',str(args.output)],cwd=ROOT,capture_output=True,text=True)
        assert result.returncode and reason in result.stderr,result.stderr
    valid=copy.deepcopy(definition)
    for change,reason in [(dict(capacity=0),'capacity'),(dict(kind='mesh'),'model')]:
        definition=copy.deepcopy(valid)
        definition['emitters'][0].update(change)
        effect.write_text(json.dumps(definition))
        rejected(reason)
    definition=copy.deepcopy(valid)
    definition['emitters'].append(copy.deepcopy(definition['emitters'][0]))
    effect.write_text(json.dumps(definition))
    rejected('duplicate')
print('PASS: effect schema, fixed cooked layout, content hash/index, incremental edits and authoring diagnostics')
