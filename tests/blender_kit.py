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
            assert (output/'source'/name/(name+'_lod1.gltf')).is_file(),name
            assert (output/'source'/name/'baked.png').is_file(),name
            model=output/'cooked/models/theme'/(name+'.iqm')
            assert model.read_bytes()[:16]==b'INTERQUAKEMODEL\0',name
            assert struct.unpack_from('<I',model.read_bytes(),16)[0]==2
        check=subprocess.run([sys.executable,'tools/assets','validate',str(output)],cwd=ROOT,text=True,capture_output=True)
        assert check.returncode==0,check.stderr
        return {p.relative_to(output).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
                for p in output.rglob('*') if p.is_file() and p.name!='blender.log'}
    assert build(root/'a')==build(root/'b'), 'headless procedural kit differs across fresh builds'
print('PASS: pinned Blender reference kit, real UVs/LOD/bakes, native cooker output and complete licensed provenance')
