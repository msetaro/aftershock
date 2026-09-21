"""Orthographic class projection of generated IBSP triangles, aligned to sketch pixels."""
import hashlib
import json
import struct

import cv2
import numpy as np
from PIL import Image


COLORS = dict(building=(190,80,70),solid=(210,150,50),wall=(110,160,190),
              platform=(110,180,90),overhead=(170,110,210),transition=(70,170,170),unmapped=(255,0,255))


def triangles(data):
    if struct.unpack_from('<4sI',data)!=(b'IBSP',46):
        raise ValueError('overhead requires compiled IBSP 46')
    def lump(index):
        start,length = struct.unpack_from('<ii',data,8+index*8)
        return data[start:start+length]
    shaders = [name.split(b'\0',1)[0].decode() for name,_,_ in struct.iter_unpack('<64sii',lump(1))]
    draw_vertices = lump(10)
    vertices = np.array([v[:3] for v in struct.iter_unpack('<10f4B',draw_vertices)])
    indices = [i[0] for i in struct.iter_unpack('<i',lump(11))]
    surfaces = lump(13)
    for offset in range(0,len(surfaces),104):
        shader,_,kind,first,count,start,length = struct.unpack_from('<7i',surfaces,offset)
        if kind not in (1,3):
            continue
        for i in range(start,start+length,3):
            xyz = vertices[[first+index for index in indices[i:i+3]]]
            # Surface normals are stored explicitly; projected vertical faces
            # have no area. Ignore undersides, including the enclosing sky slab.
            normal_z = struct.unpack_from('<f',draw_vertices,(first+indices[i])*44+36)[0]
            if normal_z>0.01:
                yield shaders[shader],xyz


def compare(data,interpretation,output,threshold=.9):
    width,height = interpretation['image_size']
    scale = interpretation['scale']
    if not (0<threshold<=1 and width*height<=32*1024*1024 and scale>0):
        raise ValueError('invalid overhead dimensions, scale or threshold')
    marks = [m for m in interpretation['marks'] if m['classification']=='geometry' and m.get('pixel_polygon')]
    identities = {m['id']:m['kind'] for m in marks}
    expected,actual = {},{}
    def mask(collection,kind):
        return collection.setdefault(kind,np.zeros((height,width),dtype=np.uint8))
    for mark in marks:
        cv2.fillPoly(mask(expected,mark['kind']),[np.rint(mark['pixel_polygon']).astype(np.int32)],1)
    count = 0
    for shader,xyz in triangles(data):
        if shader.startswith('textures/s/'):
            identity = shader.split('/')[2]
            kind = identities.get(identity,'unmapped')
        elif shader=='textures/level/fence':
            kind = 'wall'
        else:
            continue
        points = np.rint(np.column_stack((xyz[:,0]/scale+width/2,height/2-xyz[:,1]/scale))).astype(np.int32)
        cv2.fillPoly(mask(actual,kind),[points],1)
        count += 1
    if not count:
        raise ValueError('compiled BSP has no labeled sketch surface triangles')
    classes = {}
    compiled = np.full((height,width,3),240,dtype=np.uint8)
    difference = compiled.copy()
    for kind in sorted(expected.keys()|actual.keys()):
        a,b = mask(expected,kind).astype(bool),mask(actual,kind).astype(bool)
        union = int((a|b).sum())
        intersection = int((a&b).sum())
        classes[kind] = dict(iou=intersection/union if union else 1,expected_pixels=int(a.sum()),
                             compiled_pixels=int(b.sum()),intersection_pixels=intersection,union_pixels=union)
        compiled[b] = COLORS.get(kind,COLORS['unmapped'])
        difference[a&b] = (100,180,100)
        difference[a&~b] = (230,70,70)
        difference[b&~a] = (60,100,230)
    output.mkdir(parents=True,exist_ok=True)
    Image.fromarray(compiled).save(output/'compiled-overhead.png')
    Image.fromarray(difference).save(output/'overhead-difference.png')
    report = dict(method='CPU orthographic projection of compiled upward-facing BSP triangles',
                  alignment='inverse of recorded sketch scale and image-center transform',
                  bsp_sha256=hashlib.sha256(data).hexdigest(),triangles=count,classes=classes,threshold=threshold,
                  passed=bool(classes) and all(c['iou']>=threshold for c in classes.values()),
                  overhead='compiled-overhead.png',difference='overhead-difference.png')
    (output/'overhead.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    return report
