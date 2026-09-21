#!/usr/bin/env python3
"""Check offline reflection directions, GGX filtering and the owned atlas format."""
import importlib.util
import math
from pathlib import Path
import struct
import tempfile

from PIL import Image
from run import ROOT

spec = importlib.util.spec_from_file_location('probe_bake', ROOT/'tools/level/probes.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
for face in range(6):
    for u,v in ((0,0),(-.9,.7),(.7,-.9)):
        direction = module.direction(face,u,v)
        actual,a,b = module.face_uv(direction)
        assert actual == face and abs(a-u)<1e-6 and abs(b-v)<1e-6
assert module.direction(0,0,0) == (1,0,0)
assert module.direction(0,1,0)[1] < 0
assert module.direction(4,0,-1)[0] < 0
faces = [Image.new('RGB',(16,16),(128,64,32)) for _ in range(6)]
atlas = module.prefilter(faces,16,32)
assert atlas.size == (96,80)
# Filtering operates on linear radiance, not display bytes.
expected = tuple(round(module.linear(c/255)*255) for c in (128,64,32)) + (255,)
assert set(atlas.getdata()) == {expected}
colored = [Image.new('RGB',(16,16),color) for color in
           ((255,0,0),(0,255,0),(0,0,255),(255,255,0),(255,0,255),(0,255,255))]
a = module.prefilter(colored,16,32)
b = module.prefilter(colored,16,32)
assert a.tobytes() == b.tobytes()
for face,color in enumerate(((255,0,0),(0,255,0),(0,0,255),(255,255,0),(255,0,255),(0,255,255))):
    assert a.getpixel((face*16+8,8)) == color+(255,)
sharp = list(a.crop((0,0,16,16)).getdata())
rough = list(a.crop((0,64,16,80)).getdata())
assert sum(c[0] for c in rough) < sum(c[0] for c in sharp)
assert sum(c[1]+c[2] for c in rough) > 0
with tempfile.TemporaryDirectory() as temporary:
    path = Path(temporary)/'owned.asprobe'
    module.write_probes(path,[{'origin':[1,2,3],'radius':256}], [atlas])
    data = path.read_bytes()
    assert struct.unpack_from('<8sIIII',data) == (b'ASPROBE\0',1,1,16,5)
    assert struct.unpack_from('<4f',data,24) == (1,2,3,256)
    assert data[40:] == atlas.tobytes()
print('PASS: six engine camera directions, linear GGX roughness filtering and atlas bytes')
