"""Assemble validated CC0 kits and place deterministic non-solid dressing."""
import copy
import datetime
import hashlib
import json
import math
import random
from pathlib import Path
import shutil

import shapely
from shapely import Point,LineString
from PIL import Image

from polygons import footprint,polygon,prop_bounds,opening_polygon,ruled_openings
from tools.assets.manifest import validate,digest


def assemble(level,theme,library,modules,output,seed):
    if output.exists():
        raise ValueError('theme output already exists; use a fresh directory')
    if theme.get('version')!=1 or theme.get('license')!='CC0-1.0' or type(seed) is not int:
        raise ValueError('theme version, CC0 license and integer seed are required')
    validate(library)
    validate(modules)
    assets=output/'assets'
    assets.mkdir(parents=True)
    entries=[]
    for label,root in [('materials',library),('modules',modules)]:
        manifest=json.loads((root/'manifest.json').read_text())
        for source_entry in manifest['assets']:
            entry=copy.deepcopy(source_entry)
            for kind in ('originals','cooked'):
                entry[kind]=[]
                for record in source_entry[kind]:
                    name=record['path']
                    if name.startswith('source/'):
                        target='source/'+label+'/'+name.removeprefix('source/')
                    elif name.startswith(('cooked/textures/','cooked/models/')):
                        target=name.removeprefix('cooked/')
                    else:
                        continue  # Combined runtime catalogs are rebuilt per authored map.
                    path=assets/target
                    path.parent.mkdir(parents=True,exist_ok=True)
                    shutil.copyfile(root/name,path)
                    entry[kind].append(dict(path=target,sha256=record['sha256']))
            entries.append(entry)
    def alias(source,target):
        owner=next(e for e in entries if any(f['path']==source for kind in ('originals','cooked') for f in e[kind]))
        dest=assets/target
        dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(assets/source,dest)
        owner['cooked'].append(dict(path=target,sha256=digest(dest)))
    for role in ('ground','facade'):
        alias('source/materials/'+role+'/color.png','textures/theme/'+role+'.png')
    alias('source/modules/crate/baked.png','textures/theme/prop.png')
    alias('models/theme/crate_material0.asmat','textures/theme/prop.asmat')
    for rule in theme['props']:
        if rule['material'].startswith('kit_'):
            name=theme['materials'][rule['material']]
            alias('source/modules/'+rule['module']+'/baked.png','textures/theme/'+name+'.png')
            alias('models/theme/'+rule['module']+'_material0.asmat','textures/theme/'+name+'.asmat')
    for module in json.loads((modules/'source/kit.json').read_text())['modules']:
        name=module['name']
        alias(f'source/modules/{name}/{name}.obj',f'models/theme/{name}.obj')
    source=assets/'source/theme.json'
    source.write_text(json.dumps(theme,sort_keys=True,indent=2)+'\n')
    sky=assets/'textures/theme/sky.png'
    Image.new('RGB',(16,16),tuple(theme['sky_color'])).save(sky)
    entries.append(dict(id='aftershock_theme',author='Aftershock',license='CC0-1.0',retrieved=datetime.date.today().isoformat(),
                        source_url='https://github.com/msetaro/aftershock/blob/main/tools/level/themes/manhattan.json',
                        attribution='Original Aftershock theme parameters and constant sky color',
                        originals=[dict(path='source/theme.json',sha256=digest(source))],
                        cooked=[dict(path='textures/theme/sky.png',sha256=digest(sky))]))
    (assets/'manifest.json').write_text(json.dumps(dict(version=1,assets=entries),sort_keys=True,indent=2)+'\n')
    validate(assets)
    result=copy.deepcopy(level)
    result['materials']={role:'theme/'+name for role,name in theme['materials'].items()}
    result['lighting']=copy.deepcopy(theme['lighting'])
    result['texture_scale']=copy.deepcopy(theme.get('texture_scale',{}))
    decisions=[]
    plaza=polygon(level['boundary']['polygon'],level['boundary'].get('holes',[])).centroid
    for item in result['shapes']:
        if item['kind']!='building' or 'openings' in item:
            continue
        p=footprint(item['shape'])
        points=list(p.exterior.coords)
        candidates=[]
        for edge,(a,b) in enumerate(zip(points,points[1:])):
            length=math.dist(a,b)
            dx,dy=b[0]-a[0],b[1]-a[1]
            facing=((plaza.x-(a[0]+b[0])/2)*dy-(plaza.y-(a[1]+b[1])/2)*dx)/length
            if length>=128 and facing>0:
                candidates.append((facing,edge,length))
        if not candidates:
            raise ValueError('no doorway-sized wall faces the plaza: '+item['id']+'; supply an explicit opening')
        _,edge,length=max(candidates)
        item['openings']=[dict(edge=edge,at=length/2,width=96,sill=0,height=96)]
        decisions.append(dict(id=item['id'],decision='96-unit doorway centered on the wall facing the boundary centroid',opening=item['openings'][0]))
    placement=theme['placement']
    if not (0<=placement['density']<=1 and 64<=placement['spacing']<=2048 and
            all(32<=placement[k]<=512 for k in ('door_clearance','spawn_clearance','lane_clearance'))):
        raise ValueError('theme placement density/clearances outside limits')
    shapes=[(s,footprint(s['shape'])) for s in result['shapes']]
    keep=[]
    for shape,p in shapes:
        if shape['kind']=='building':
            for opening in ruled_openings(shape,p,shape.get('floors',1)):
                if opening['sill']==0:
                    keep.append(opening_polygon(p,opening,shape.get('wall_thickness',16)).buffer(placement['door_clearance']))
    keep.extend(Point(*s['origin'][:2]).buffer(placement['spawn_clearance']) for s in level['spawns'])
    keep.extend(LineString(i['points']).buffer(max(placement['lane_clearance'],i.get('width',0)/2))
                for i in level.get('intents',[]) if i['kind']=='route')
    forbidden=shapely.union_all(keep)
    solid=shapely.union_all([p for s,p in shapes if s['kind'] in ('building','solid','wall')])
    area=polygon(level['boundary']['polygon'],level['boundary'].get('holes',[])).buffer(-16)
    catalog={m['name']:m for m in json.loads((modules/'source/kit.json').read_text())['modules']}
    placed=[]
    for item,p in shapes:
        if item['kind']!='building':
            continue
        # A stable per-building stream means an edit never re-rolls other buildings.
        rng=random.Random(hashlib.sha256(f"{seed}/{item['id']}".encode()).digest())
        points=list(p.exterior.coords)
        for edge,(a,b) in enumerate(zip(points,points[1:])):
            length=math.dist(a,b)
            dx,dy=(b[0]-a[0])/length,(b[1]-a[1])/length
            count=max(1,int(length/placement['spacing']))
            for index in range(count):
                if rng.random()>placement['density']:
                    continue
                along=(index+.5)*length/count
                for rule in theme['props']:
                    module=catalog[rule['module']]
                    low,high=module['bounds']
                    size=[math.ceil(B-A) for A,B in zip(low,high)]
                    center=[A+s/2 for A,s in zip(low,size)]
                    yaw=math.degrees(math.atan2(dy,dx))
                    x,y=a[0]+dx*along+dy*rule['offset'],a[1]+dy*along-dx*rule['offset']
                    z=math.ceil(item['base']+rule['height']-min(0,low[2]))
                    identity='p_'+hashlib.sha256(f"{item['id']}/{edge}/{index}/{rule['module']}".encode()).hexdigest()[:20]
                    prop=dict(id=identity,model='models/theme/'+rule['module']+'.obj',origin=[round(x),round(y),round(z)],
                              size=size,bounds_center=center,angle=round(yaw,6),material=rule['material'],solid=False)
                    footprint_,bottom,top=prop_bounds(prop)
                    if not area.covers(footprint_) or footprint_.intersects(forbidden) or footprint_.intersects(solid):
                        continue
                    if bottom<level['boundary']['floor']-.001 or top>level['boundary']['ceiling']:
                        continue
                    if any(footprint_.intersects(q) and bottom<Z and top>z for q,z,Z in placed):
                        continue
                    # Do not cover the current building's upper storey/roof openings.
                    placed.append((footprint_,bottom,top))
                    result['props'].append(prop)
    if len(result['props'])>128:
        raise ValueError('theme exceeds the 128-prop level budget; reduce density')
    (output/'assembly.json').write_text(json.dumps(dict(seed=seed,decisions=decisions),sort_keys=True,indent=2)+'\n')
    (output/'level.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    return result
