#!/usr/bin/env python3
"""Pinned headless Blender builds reproducible UV/LOD/baked props and native models."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from run import ROOT
from shapely import Polygon
sys.path.insert(0,str(ROOT/'tools/cook'))
from gltf import Document

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--blender',type=Path)
args=parser.parse_args()
with tempfile.TemporaryDirectory(prefix='aftershock-blender-kit-') as temporary:
    root=Path(temporary)
    params=root/'parameters.json'
    params.write_text(json.dumps(dict(version=1,seed=164,bay_width=4,storey_height=4,wall_thickness=.5)))
    def build(output):
        command=[sys.executable,'tools/blender','kit','--parameters',str(params),'--out',str(output)]
        if args.blender:
            command+=['--blender',str(args.blender)]
        result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
        assert result.returncode==0,result.stderr
        report=json.loads(result.stdout)
        assert report['ok'] and report['blender']=='5.0.1'
        manifest=json.loads((output/'manifest.json').read_text())
        assert manifest['assets'] and all(a['generator']['version']=='5.0.1' for a in manifest['assets'])
        for name in ('facade','doorway','cornice','curb','stairs','fence','barrier','crate','sign'):
            gltf=json.loads((output/'source'/name/(name+'.gltf')).read_text())
            primitives=[p for m in gltf['meshes'] for p in m['primitives']]
            assert primitives and all('TEXCOORD_0' in p['attributes'] for p in primitives),name
            document=Document(output/'source'/name/(name+'.gltf'),lambda path:path.read_bytes())
            tops=[]
            for primitive in primitives:
                positions=document.accessor(primitive['attributes']['POSITION'])
                normals=document.accessor(primitive['attributes']['NORMAL'])
                indices=[v[0] for v in document.accessor(primitive['indices'])]
                for i in range(0,len(indices),3):
                    tri=indices[i:i+3]
                    if all(normals[k][1]>.99 for k in tri):
                        tops.append((round(positions[tri[0]][1],5),Polygon([(positions[k][0],positions[k][2]) for k in tri])))
            for i,(height,triangle) in enumerate(tops):
                assert all(height!=other_height or triangle.intersection(other).area<1e-7
                           for other_height,other in tops[i+1:]), ('coplanar overlap',name)
            assert (output/'source'/name/(name+'_lod1.gltf')).is_file(),name
            assert (output/'source'/name/'baked.png').is_file(),name
            # q3map2's OBJ importer maps (x,y,z) to (x,-z,y).
            vertices=[(float(v[1]),-float(v[3]),float(v[2]))
                      for line in (output/'source'/name/(name+'.obj')).read_text().splitlines()
                      if (v:=line.split()) and v[0]=='v']
            actual=[[min(v[k] for v in vertices) for k in range(3)],
                    [max(v[k] for v in vertices) for k in range(3)]]
            expected=next(m['bounds'] for m in report['modules'] if m['name']==name)
            assert all(abs(a-b)<.001 for aa,bb in zip(actual,expected) for a,b in zip(aa,bb)), ('OBJ/native axes differ',name,actual,expected)

            model=output/'cooked/models/theme'/(name+'.iqm')
            assert model.read_bytes()[:16]==b'INTERQUAKEMODEL\0',name
            assert struct.unpack_from('<I',model.read_bytes(),16)[0]==2
        check=subprocess.run([sys.executable,'tools/assets','validate',str(output)],cwd=ROOT,text=True,capture_output=True)
        assert check.returncode==0,check.stderr
        return {p.relative_to(output).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
                for p in output.rglob('*') if p.is_file() and p.name!='blender.log'}
    assert build(root/'a')==build(root/'b'), 'headless procedural kit differs across fresh builds'
print('PASS: pinned Blender reference kit, real UVs/LOD/bakes, native cooker output and complete licensed provenance')
