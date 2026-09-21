"""Offline meshoptimizer LODs; compact existing vertices without altering their data."""
from functools import lru_cache
import hashlib
import json
import math
import os
import struct
import subprocess

from texture import ROOT,encoder


@lru_cache(maxsize=1)
def simplifier():
    vendor=ROOT/'third_party/meshoptimizer'
    provenance=json.loads((vendor/'provenance.json').read_text())
    for name,expected in provenance['files'].items():
        if hashlib.sha256((vendor/name).read_bytes()).hexdigest()!=expected:
            raise ValueError('meshoptimizer source differs from pinned provenance: '+name)
    return encoder().with_name('aftershock-cook-lod'+('.exe' if os.name=='nt' else ''))


def ratios(options):
    result=options.get('lod_ratios',[])
    if not isinstance(result,list) or len(result)>3:
        raise ValueError('lod_ratios must contain at most three decreasing fractions')
    previous=1
    for ratio in result:
        if isinstance(ratio,bool) or not isinstance(ratio,(int,float)) or not math.isfinite(ratio) or not 0<ratio<previous:
            raise ValueError('lod_ratios must be finite, positive and strictly decreasing below one')
        previous=ratio
    error=options.get('lod_error',.01)
    if isinstance(error,bool) or not isinstance(error,(int,float)) or not math.isfinite(error) or not 0<=error<=1:
        raise ValueError('lod_error must be finite in [0,1] relative to mesh extent')
    return result,error


def simplify(vertices,triangles,meshes,ratio,error):
    output_vertices,output_triangles,output_meshes=[],[],[]
    for name,material,first,count,first_face,face_count in meshes:
        source=vertices[first:first+count]
        faces=[[index-first for index in face] for face in triangles[first_face:first_face+face_count]]
        # Joint identities are categorical: retain vertices across changes of influence set.
        locks=bytearray(count)
        for face in faces:
            if len({tuple(source[index][3]) for index in face})>1:
                for index in face:
                    locks[index]=1
        data=bytearray(struct.pack('<3If',count,face_count*3,max(1,int(face_count*ratio))*3,error))
        for position,normal,uv,joints,weights,tangent,color in source:
            data.extend(struct.pack('<12f',*position,*normal,*uv,*weights))
        data.extend(locks)
        data.extend(struct.pack('<'+'I'*face_count*3,*[index for face in faces for index in face]))
        result=subprocess.run([str(simplifier())],input=data,capture_output=True,check=True).stdout
        size=struct.unpack_from('<I',result)[0]
        if not size or size%3 or size>face_count*3 or len(result)!=4+size*4:
            raise ValueError('meshoptimizer returned an invalid triangle count')
        indices=struct.unpack_from('<'+'I'*size,result,4)
        if any(index>=count for index in indices):
            raise ValueError('meshoptimizer returned an invalid vertex index')
        # Stable original order retains every normal/UV/skin/color/tangent byte.
        used=sorted(set(indices))
        base=len(output_vertices)
        remap={index:base+i for i,index in enumerate(used)}
        output_meshes.append((name,material,base,len(used),len(output_triangles),size//3))
        output_vertices.extend(source[index] for index in used)
        output_triangles.extend([[remap[index] for index in indices[i:i+3]] for i in range(0,size,3)])
    return output_vertices,output_triangles,output_meshes
