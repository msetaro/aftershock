#!/usr/bin/env python3
"""Cook smaller native mesh LODs while retaining the original and its skeleton."""
import argparse
import hashlib
import json
import shlex
import subprocess
from pathlib import Path
import struct
import tempfile

from cook import cook,model_header,source_assets
from run import ROOT, SCRATCH

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cxx',default='c++')
parser.add_argument('--output',type=Path,default=SCRATCH/'aftershock-lod')
args=parser.parse_args()
args.output=args.output.resolve()
args.output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='aftershock-lod-source-') as temporary:
    source=Path(temporary)
    project,_=source_assets(source)
    document=json.loads((source/'rig.gltf').read_text())
    blob=bytearray((source/'rig.bin').read_bytes())
    def array(values,fmt,component,shape,count):
        blob.extend(bytes(-len(blob)%4))
        data=struct.pack('<'+fmt*len(values),*values)
        views=document['bufferViews']
        views.append(dict(buffer=0,byteOffset=len(blob),byteLength=len(data)))
        blob.extend(data)
        accessors=document['accessors']
        accessors.append(dict(bufferView=len(views)-1,componentType=component,type=shape,count=count))
        return len(accessors)-1
    side=17
    positions=[v for y in range(side) for x in range(side) for v in (x/16,y/8,0)]
    uv=[v for y in range(side) for x in range(side) for v in (x/16,y/16)]
    faces=[]
    for y in range(side-1):
        for x in range(side-1):
            a=y*side+x
            faces.extend((a,a+1,a+side,a+1,a+side+1,a+side))
    count=side*side
    attributes=dict(POSITION=array(positions,'f',5126,'VEC3',count),
                    NORMAL=array([0,0,1]*count,'f',5126,'VEC3',count),
                    TEXCOORD_0=array(uv,'f',5126,'VEC2',count),
                    JOINTS_0=array([0,1,0,0]*count,'B',5121,'VEC4',count),
                    WEIGHTS_0=array([v for y in range(side) for x in range(side) for v in (1-y/16,y/16,0,0)],'f',5126,'VEC4',count))
    indices=array(faces,'H',5123,'SCALAR',len(faces))
    document['meshes']=[dict(name='grid',primitives=[dict(attributes=attributes,indices=indices)])]
    document['buffers'][0]['byteLength']=len(blob)
    (source/'rig.bin').write_bytes(blob)
    (source/'rig.gltf').write_text(json.dumps(document))
    recipe=dict(name='models/grid',kind='model',source='rig.gltf',scale=1,fps=2)
    project.write_text(json.dumps(dict(version=1,assets=[recipe])))
    cook(project,args.output/'reference')
    base=(args.output/'reference/models/grid.iqm').read_bytes()
    original=model_header(base)
    assert original[8]==count and original[10]==len(faces)//3
    recipe['lod_ratios']=[.5,.25]
    project.write_text(json.dumps(dict(version=1,assets=[recipe])))
    cook(project,args.output/'lod')
    def geometry(data):
        header=model_header(data)
        metadata=struct.unpack_from('<4I',data,header[26])[2]
        # The recipe changes both provenance hashes, but no full-detail geometry.
        return data[:metadata+4]+bytes(64)+data[metadata+68:]
    assert geometry((args.output/'lod/models/grid.iqm').read_bytes())==geometry(base),'LOD cooking must retain the full-detail geometry and animation'
    manifest=(args.output/'lod/models/grid.aslod').read_bytes()
    magic,version,size,hashed=struct.unpack_from('<8sII32s',manifest)
    assert magic==b'ASLOD\0\0\0' and version==1 and size==36+2*96
    assert hashlib.sha256(manifest[48:]).digest()==hashed
    assert manifest[48:80]==hashlib.sha256((args.output/'lod/models/grid.iqm').read_bytes()).digest()
    assert struct.unpack_from('<I',manifest,80)[0]==2
    previous=original[10]
    for level,ratio in enumerate(recipe['lod_ratios'],1):
        data=(args.output/f'lod/models/grid_lod{level}.iqm').read_bytes()
        record_path,record_hash=struct.unpack_from('<64s32s',manifest,84+(level-1)*96)
        assert record_path.split(b'\0',1)[0].decode()==f'models/grid_lod{level}.iqm'
        assert record_hash==hashlib.sha256(data).digest()
        header=model_header(data)
        assert 0<header[10]<previous and header[10]<=original[10]*ratio
        assert header[8]<original[8],'remove vertices no longer referenced by the LOD'
        assert header[13]==original[13] and header[17]==original[17] and header[19]==original[19]
        # Joint transforms, poses and compressed animation remain identical.
        for count_index,offset_index,stride in ((13,14,48),(15,16,88),(19,21,2*original[20])):
            length=original[count_index]*stride
            assert data[header[offset_index]:header[offset_index]+length]==base[original[offset_index]:original[offset_index]+length]
        previous=header[10]
    hashes={p.relative_to(args.output/'lod').as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
            for p in (args.output/'lod').rglob('*.iqm')}
    assert cook(project,args.output/'lod')['built']==[]
    cook(project,args.output/'repeat')
    assert hashes=={p.relative_to(args.output/'repeat').as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
                    for p in (args.output/'repeat').rglob('*.iqm')}
probe=args.output/'lod-probe'
subprocess.run([*shlex.split(args.cxx),'-std=c++20','-O2','-fno-exceptions','-fno-rtti',
                '-DUSE_VULKAN_API','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
                '-ffp-contract=off','-fno-fast-math','-fsanitize=undefined','-fno-sanitize-recover=all',
                'tests/probes/lod.cpp','engine/qcommon/q_shared.cpp','engine/qcommon/q_math.cpp',
                '-Wl,--gc-sections','-o',str(probe)],cwd=ROOT,check=True)
subprocess.run([str(probe),*[str(args.output/f'lod/models/grid{suffix}.iqm') for suffix in ('','_lod1','_lod2')]],check=True)
print('PASS: compact mesh LODs, retained original/skeleton/animation and repeated deterministic cooking')
