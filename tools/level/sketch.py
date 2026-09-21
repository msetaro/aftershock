"""Measure an agent-interpreted drawing; preserve readings, IDs and uncertainty."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import sys

import cv2
import numpy as np
from PIL import Image, ImageDraw
from shapely import Polygon, Point
from shapely.geometry.polygon import orient


def require(condition,message):
    if not condition:
        raise ValueError(message)


def rgb(value):
    require(isinstance(value,str) and len(value)==7 and value.startswith('#'), 'key colors use #rrggbb')
    return np.array([int(value[i:i+2],16) for i in (1,3,5)],dtype=np.float32)


def measure(image,notes,previous=None):
    require(notes.get('version')==1, 'notes version must be 1')
    scale = notes['scale']['units']/notes['scale']['pixels']
    require(math.isfinite(scale) and .01<=scale<=128, 'scale must be finite and in .01..128 units/pixel')
    require(image.width*image.height<=32*1024*1024, 'drawing exceeds 32 megapixels')
    original_size = image.size
    pixels = np.asarray(image.convert('RGB'))
    transform = None
    if 'corners' in notes:
        corners = np.array(notes['corners'],dtype=np.float32)
        require(corners.shape==(4,2) and np.isfinite(corners).all(), 'perspective corners must be four finite image coordinates')
        width,height = notes.get('rectified_size',image.size)
        require(64<=width<=8192 and 64<=height<=8192 and width*height<=32*1024*1024, 'rectified image size outside limits')
        transform = cv2.getPerspectiveTransform(corners,np.array([[0,0],[width-1,0],[width-1,height-1],[0,height-1]],dtype=np.float32))
        pixels = cv2.warpPerspective(pixels,transform,(width,height))
        image = Image.fromarray(pixels)
    def world(point):
        return [round((point[0]-image.width/2)*scale,4),round((image.height/2-point[1])*scale,4)]
    marks = copy.deepcopy(notes.get('marks',[]))
    require(len(marks)<=512 and len({m['id'] for m in marks})==len(marks), 'mark IDs must be unique; maximum 512')
    require(all(m['classification'] in ('geometry','annotation','intent') for m in marks), 'unknown mark classification')
    assumptions = []
    for mark in marks:
        mark.setdefault('reading',mark.get('text','Explicit agent interpretation'))
        mark.setdefault('confidence',.5)
        if mark['confidence']<.75:
            assumptions.append(dict(id=mark['id'],reason=mark['reading'],decision=mark.get('kind',mark['classification'])))
    available = np.ones(pixels.shape[:2],dtype=np.uint8)*255
    for mark in marks:
        if mark['classification']=='annotation' and 'region' in mark:
            x,y,X,Y = map(int,mark['region'])
            cv2.rectangle(available,(x,y),(X,Y),0,-1)
    keys = notes.get('key',[])
    require(keys and len(keys)<=32, 'provide the drawing key read by the agent (up to 32 colors)')
    gap = notes.get('gap_pixels',5)
    simplify = notes.get('simplify_pixels',2)
    require(type(gap) is int and 1<=gap<=31 and 0<simplify<=32, 'gap/simplification limits exceeded')
    kernel = np.ones((gap,gap),np.uint8)
    candidates = []
    explained = np.zeros_like(available)
    for key in keys:
        color = rgb(key['color'])
        mask = (np.linalg.norm(pixels.astype(np.float32)-color,axis=2)<=notes.get('color_distance',65)).astype(np.uint8)*255
        explained |= mask
        if key['classification']!='geometry':
            continue
        mask &= available
        mask = cv2.morphologyEx(mask,cv2.MORPH_CLOSE,kernel)
        contours,_ = cv2.findContours(mask,cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_SIMPLE)
        for contour in contours:
            if cv2.contourArea(contour)<notes.get('min_area_pixels',64):
                continue
            perimeter = cv2.arcLength(contour,True)
            poly = cv2.approxPolyDP(contour,simplify,True).reshape(-1,2).tolist()
            if len(poly)<3:
                continue
            polygon = Polygon(poly)
            if not polygon.is_valid:
                assumptions.append(dict(id='unresolved_'+str(len(assumptions)+1),reason='Self-crossing measured contour',decision='omit until a per-mark geometry override resolves it'))
                continue
            rectangle = cv2.boxPoints(cv2.minAreaRect(contour)).tolist()
            box = Polygon(rectangle)
            if len(poly)<=6 and polygon.area/box.area>.9:
                poly = rectangle
            candidates.append(dict(pixel_polygon=poly,key=key,center=list(Polygon(poly).centroid.coords)[0],perimeter=perimeter))
    require(0<len(candidates)<=128, 'drawing must yield 1..128 geometry regions; refine key/mark regions')
    candidates.sort(key=lambda c:(round(c['center'][1]/16),c['center'][0]))
    used = set()
    shapes = []
    previous_marks = (previous or {}).get('marks',[])
    for index,candidate in enumerate(candidates,1):
        key = candidate['key']
        poly = Polygon(candidate['pixel_polygon'])
        explicit = [m for m in marks if m['classification']=='geometry' and 'region' in m and
                    m['region'][0]<=candidate['center'][0]<=m['region'][2] and m['region'][1]<=candidate['center'][1]<=m['region'][3]]
        require(len(explicit)<=1, 'geometry regions overlap; use nonoverlapping mark associations')
        prior = []
        for old in previous_marks:
            if old['classification']=='geometry' and old.get('pixel_polygon') and old['id'] not in used:
                p = Polygon(old['pixel_polygon'])
                overlap = poly.intersection(p).area/poly.union(p).area
                if overlap>.5:
                    prior.append((overlap,old))
        identity = explicit[0]['id'] if explicit else (max(prior,key=lambda pair:pair[0])[1]['id'] if prior else key['kind']+'_'+str(index))
        require(identity not in used, 'multiple contours associated with '+identity+'; specify a shape split/merge in notes')
        used.add(identity)
        mark = explicit[0] if explicit else dict(id=identity,classification='geometry',confidence=.8,reading=key.get('reading','Drawing key '+key['color']))
        if not explicit:
            marks.append(mark)
        mark.update(pixel_polygon=candidate['pixel_polygon'],color=key['color'],kind=key['kind'])
        footprint = orient(Polygon([world(p) for p in candidate['pixel_polygon']]),sign=1)
        kind = key['kind']
        item = dict(id=identity,kind=kind,shape=dict(polygon=[list(p) for p in footprint.exterior.coords[:-1]]),
                    base=key.get('base',0),height=key.get('height',128*key.get('floors',1)))
        if kind=='building':
            item.update(floors=key.get('floors',1),wall_thickness=16)
        item.update(copy.deepcopy(notes.get('overrides',{}).get(identity,{})))
        shapes.append(item)
    for mark in marks:
        if mark['classification']=='geometry' and mark['id'] not in used:
            assumptions.append(dict(id=mark['id'],reason='No measured geometry matches this mark',decision='not compiled'))
    boundary = notes.get('boundary',[[8,8],[image.width-8,8],[image.width-8,image.height-8],[8,image.height-8]])
    if 'boundary' not in notes:
        assumptions.append(dict(id='boundary',reason='No explicit playable boundary reading',decision='use the image inset by eight pixels'))
    boundary = orient(Polygon([world(p) for p in boundary]),sign=1)
    require(boundary.is_valid and boundary.area>0, 'interpreted boundary must be a simple polygon')
    intents = []
    for mark in marks:
        if mark['classification']=='intent':
            intent = {k:copy.deepcopy(v) for k,v in mark.items() if k not in ('classification','confidence','reading','color')}
            if 'points' in intent:
                intent['points'] = [world(p) for p in intent['points']]
            intents.append(intent)
    # Unknown ink is evidence too: retain an assumption, never silently turn it into walls.
    border_mask = np.zeros_like(available)
    cv2.polylines(border_mask,[np.array(notes.get('boundary',[]),dtype=np.int32)],True,255,max(7,gap)) if notes.get('boundary') else None
    background = np.median(pixels.reshape(-1,3),axis=0)
    ink = np.linalg.norm(pixels.astype(np.float32)-background,axis=2)>60
    unexplained = ink & (explained==0) & (available!=0) & (border_mask==0)
    if int(unexplained.sum())>16:
        assumptions.append(dict(id='unclassified_ink',reason=f'{int(unexplained.sum())} ink pixels are outside the read key/annotations',decision='omit from geometry; inspect interpretation overlay'))
    level = dict(version=2,name=notes.get('name','sketch_level'),
                 materials={role:'level/'+role for role in ('floor','wall','trim','cover','prop','sky')},
                 rules=notes.get('rules',dict(min_corridor_width=64,min_door_height=80,max_sightline=8192,max_cover_gap=2048)),
                 boundary=dict(polygon=[list(p) for p in boundary.exterior.coords[:-1]],floor=0,ceiling=1024),
                 shapes=shapes,spawns=copy.deepcopy(notes.get('spawns',[])),props=[],pickups=[],
                 lighting=dict(ambient=48,lights=[]),intents=intents)
    interpretation = dict(version=1,image_size=list(image.size),original_size=list(original_size),
                          perspective=transform.tolist() if transform is not None else None,
                          scale=scale,key=keys,marks=marks,assumptions=assumptions)
    return interpretation,level,image


def overlays(image,interpretation,out):
    classified,numbered = image.copy(),image.copy()
    colors = dict(geometry='#e03030',annotation='#20a040',intent='#2070ff')
    for index,mark in enumerate(interpretation['marks'],1):
        points = mark.get('pixel_polygon',mark.get('points'))
        if not points and 'region' in mark:
            x,y,X,Y = mark['region']
            points = [[x,y],[X,y],[X,Y],[x,Y],[x,y]]
        if not points:
            continue
        points = [tuple(p) for p in points]
        for target in (classified,numbered):
            pen = ImageDraw.Draw(target)
            pen.line(points+([points[0]] if 'pixel_polygon' in mark else []),fill=colors[mark['classification']],width=2)
            label = f"{index}: {mark['id']}"
            x,y = points[0]
            pen.rectangle((x,y-13,x+len(label)*7,y),fill='white')
            pen.text((x,y-12),label,fill=colors[mark['classification']])
        mark['number'] = index
    classified.save(out/'interpretation.png')
    numbered.save(out/'overlay.png')


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sketch',type=Path,required=True)
    parser.add_argument('--notes',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args = parser.parse_args(argv)
    try:
        require(args.notes.stat().st_size<=1024*1024, 'notes exceed 1 MiB')
        notes = json.loads(args.notes.read_text())
        previous = json.loads((args.out/'interpretation.json').read_text()) if (args.out/'interpretation.json').is_file() else None
        with Image.open(args.sketch) as source:
            interpretation,level,image = measure(source,notes,previous)
        args.out.mkdir(parents=True,exist_ok=True)
        overlays(image,interpretation,args.out)
        interpretation['source_sha256'] = hashlib.sha256(args.sketch.read_bytes()).hexdigest()
        for name,data in [('interpretation.json',interpretation),('level.json',level)]:
            (args.out/name).write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')
        print(json.dumps(dict(ok=True,shapes=len(level['shapes']),assumptions=interpretation['assumptions'],output=str(args.out))))
        return 0
    except (OSError,ValueError,KeyError,TypeError,cv2.error) as exc:
        print(json.dumps(dict(ok=False,error=str(exc))),file=sys.stderr)
        return 1
