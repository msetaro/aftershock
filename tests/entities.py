#!/usr/bin/env python3
"""Cook inherited entity definitions and verify resolved pickup components."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import shlex
import struct
import tempfile
from cook import cook
from run import ROOT, SCRATCH, run

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-entities')
parser.add_argument('--cc',default='gcc')
parser.add_argument('--cxx',default='g++')
args=parser.parse_args()
with tempfile.TemporaryDirectory(prefix='aftershock-entities-') as temporary:
    source=Path(temporary)
    shutil.copyfile(ROOT/'tests/assets/entities/pickups.json',source/'pickups.json')
    project=source/'assets.json'
    project.write_text(json.dumps(dict(version=1,assets=[dict(name='entities/pickups',kind='entities',source='pickups.json')])))
    assert cook(project,args.output)['built'] in ([],['entities/pickups'])
    data=(args.output/'entities/pickups.asent').read_bytes()
    magic,version,size=struct.unpack_from('<8sII',data)
    assert (magic,version,size)==(b'ASENT\0\0\0',1,len(data)-48)
    assert hashlib.sha256(data[48:]).digest()==data[16:48]
    name,count,fields=struct.unpack_from('<32sII',data,48)
    assert name.rstrip(b'\0')==b'pickup_definitions' and count==3
    def string(value):
        return value.split(b'\0',1)[0].decode('ascii')
    records={}
    for index in range(count):
        identity,native,first,length,mask,priority,radius=struct.unpack_from('<64s64sIIIIf',data,88+148*index)
        properties={}
        for offset in range(first,first+length):
            component,key,value=struct.unpack_from('<16s32s128s',data,88+148*count+176*offset)
            properties[string(key)]=(string(component),string(value))
        records[string(identity)]=(string(native),properties,priority,radius)
    base,crate,small=(records[key] for key in ('health_base','medical_crate','medical_crate_small'))
    assert base[0]==crate[0]==small[0]=='item_health_large'
    assert base[1]['count']==('pickup','50') and crate[1]['count']==('pickup','75')
    assert small[1]['count']==('pickup','15')
    assert crate[1]['origin']==small[1]['origin']==('transform','16 32 48')
    assert crate[1]['targetname']==small[1]['targetname']==('hooks','medical_crate')
    assert base[2:]==crate[2:]==small[2:]==(2,0)
    assert cook(project,args.output)['built']==[]
    sha,probe=args.output/'sha.o',args.output/'probe'
    run([*shlex.split(args.cc),'-std=c99','-O2','-c','third_party/sha256/sha-256.c','-o',sha])
    run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
         '-Wall','-Wextra','-Werror','-Wconversion','-Wshadow','-fsanitize=undefined','-fno-sanitize-recover=all',
         'tests/probes/entities.cpp','engine/entities/entities.cpp','engine/qcommon/state.cpp',sha,'-o',probe])
    shutil.copyfile(ROOT/'tests/assets/entities/composed.json',source/'composed.json')
    project.write_text(json.dumps(dict(version=1,assets=[dict(name='entities/composed',kind='entities',source='composed.json')])))
    assert cook(project,args.output)['built'] in ([],['entities/composed'])
    run([probe,args.output/'entities/pickups.asent',args.output/'entities/composed.asent'])
print('PASS: inherited pickup definitions, component metadata and derived replication policy')
