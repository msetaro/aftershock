#!/usr/bin/env python3
"""Cook authored post settings and check the native record without game state."""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import struct
import tempfile

from cook import cook
from run import SCRATCH,run

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc',default='gcc')
parser.add_argument('--cxx',default='g++')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-post')
args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-post-') as temporary:
    source=Path(temporary)
    definition=dict(version=1,name='filmic',exposure_ev=1,sharpen=.25,vignette=.125,grain=0,
                    lut='textures/grade.ktx2',lut_strength=.5,focus_distance=256,focus_range=128,dof_radius=0,motion_blur=0)
    profile=source/'post.json';profile.write_text(json.dumps(definition))
    project=source/'assets.json'
    project.write_text(json.dumps(dict(version=1,assets=[dict(name='post/filmic',kind='post',source='post.json')])))
    cook(project,args.output)
    path=args.output/'post/filmic.aspost';data=path.read_bytes()
    assert struct.unpack_from('<8sII',data)==(b'ASPOST\0\0',1,132)
    assert len(data)==180 and hashlib.sha256(data[48:]).digest()==data[16:48]
    assert struct.unpack_from('<9f',data,144)==(1,.25,.125,0,.5,256,128,0,0)
    assert cook(project,args.output)['built']==[]
    sha=args.output/'sha.o';probe=args.output/'probe'
    run([*shlex.split(args.cc),'-std=c99','-O2','-c','third_party/sha256/sha-256.c','-o',sha])
    run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti','-Wall','-Wextra','-Werror',
         '-fsanitize=undefined','-fno-sanitize-recover=all','-ffunction-sections','-fdata-sections',
         'tests/probes/post.cpp','engine/render/tr_cooked.cpp',sha,'-Wl,--gc-sections','-o',probe])
    run([probe,path])
    definition['exposure_ev']=-1;profile.write_text(json.dumps(definition))
    assert cook(project,args.output)['built']==['post/filmic']
    assert struct.unpack_from('<f',path.read_bytes(),144)[0]==-1
print('PASS: authored post profile layout, hash, native settings and incremental exposure edit')
