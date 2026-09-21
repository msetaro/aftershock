"""Version 1 schema and conservative design-rule checks for brush levels."""
from collections import deque
import math
import hashlib
import struct
from pathlib import PurePosixPath
import re

from geometry import room_bounds, passage, floor_at


def require(condition, message):
    if not condition:
        raise ValueError(message)


def fields(value, required, optional=()):
    require(isinstance(value, dict), 'expected an object')
    require(set(required) <= value.keys() and value.keys() <= set(required) | set(optional),
            'missing or unknown fields: ' + ', '.join(sorted(set(required)-value.keys() | value.keys()-set(required)-set(optional))))


def number(value, low=-32000, high=32000, integer=True):
    require(type(value) in ((int,) if integer else (int,float)) and math.isfinite(value) and low <= value <= high,
            f'expected {"integer" if integer else "number"} in {low}..{high}')


def vector(value, low=-32000, high=32000, integer=True):
    require(isinstance(value,list) and len(value)==3, 'expected three coordinates')
    for v in value:
        number(v,low,high,integer)


def qpath(value):
    require(isinstance(value,str) and re.fullmatch(r'[a-z0-9_./-]+',value) and len(value)<60 and
            not value.startswith('/') and all(p not in ('.','..') for p in value.split('/')), 'invalid relative asset path')
    return PurePosixPath(value)


def camera_specs(viewpoints):
    require(isinstance(viewpoints,list) and len(viewpoints)<=64, 'viewpoints must be an array of at most 64 entries')
    names = set()
    for view in viewpoints:
        fields(view,('id','origin','angles'))
        name = view['id']
        require(isinstance(name,str) and re.fullmatch(r'[a-z][a-z0-9_]{0,31}',name) and name not in names and not name.startswith('auto_'), 'invalid/duplicate/reserved viewpoint id')
        names.add(name)
        vector(view['origin'])
        vector(view['angles'],-360,360,False)
    return viewpoints


def material_sources(materials, assets):
    sources = {}
    for role,path in materials.items():
        qpath(path)
        require(path != 'level/playerclip', 'reserved material name')
        candidates = [assets / 'textures' / (path+extension) for extension in ('.tga','.png','.jpg')]
        found = next((p for p in candidates if p.is_file()),None)
        require(found is not None, 'missing material: '+path)
        require(found.resolve().is_relative_to(assets.resolve()), 'material outside asset directory')
        sources[found.relative_to(assets).as_posix()] = found
        material = assets/'textures'/(path+'.asmat')
        if material.is_file():
            require(material.resolve().is_relative_to(assets.resolve()), 'material outside asset directory')
            data = material.read_bytes()
            require(len(data)==288 and struct.unpack_from('<8sII',data)==(b'ASMAT\0\0\0',2,240) and
                    hashlib.sha256(data[48:]).digest()==data[16:48], 'level materials require valid cooked PBR v2')
            sources[material.relative_to(assets).as_posix()] = material
            for offset in (96,160,224):
                name = data[offset:offset+64].split(b'\0',1)[0].decode()
                qpath(name)
                texture = assets/name
                require(name.endswith('.ktx2') and texture.is_file() and texture.resolve().is_relative_to(assets.resolve()),
                        'missing/outside cooked material texture: '+name)
                sources[name] = texture
    return sources


def prop_asset(prop, assets, y_up=False):
    qpath(prop['model'])
    path = assets/prop['model']
    require(path.is_file() and path.resolve().is_relative_to(assets.resolve()), 'missing model or model outside assets')
    # Both versions accept self-contained OBJ geometry. No external material/texture references.
    require(path.suffix=='.obj' and path.stat().st_size<=4*1024*1024, 'props require OBJ geometry up to 4 MiB')
    require(not any(line.split() and line.split()[0] in ('mtllib','call') for line in path.read_text().splitlines()),
            'OBJ props must be self-contained without external references')
    vertices = []
    faces = 0
    for line in path.read_text().splitlines():
        tokens = line.split('#',1)[0].split()
        if not tokens:
            continue
        if tokens[0]=='v':
            require(len(tokens)==4, 'OBJ vertices require x y z')
            vertex = [float(v) for v in tokens[1:]]
            if y_up:
                vertex = [vertex[0],-vertex[2],vertex[1]]
            vector(vertex,integer=False)
            require(all(abs(v-c)<=s/2+1e-5 for v,c,s in zip(vertex,prop.get('bounds_center',[0,0,0]),prop['size'])), 'prop geometry outside declared size')
            vertices.append(vertex)
        elif tokens[0]=='f':
            require(len(tokens)>=4, 'OBJ faces require at least three vertices')
            faces += 1
        else:
            require(tokens[0] in ('vt','vn','o','g','s','usemtl'), 'unsupported OBJ directive')
    require(vertices and faces, 'prop needs vertices and faces')
    return path


def validate(level, assets):
    if level.get('version')==2:
        from polygons import validate as validate_polygons
        return validate_polygons(level,assets)
    fields(level, ('version','name','materials','rules','rooms','connections','spawns','cover','props','pickups','lighting'), ('viewpoints',))
    require(type(level['version']) is int and level['version']==1, 'level version must be 1')
    require(isinstance(level['name'],str) and re.fullmatch(r'[a-z][a-z0-9_]{0,31}',level['name']), 'invalid map name')
    fields(level['materials'], ('floor','wall','trim','cover','prop','sky'))
    sources = material_sources(level['materials'],assets)
    rules = level['rules']
    fields(rules, ('min_corridor_width','min_door_height','max_sightline','max_cover_gap'))
    for v in rules.values():
        number(v,1,64000)
    for key in ('rooms','connections','spawns','cover','props','pickups'):
        require(isinstance(level[key],list) and len(level[key])<=128, key+' must be an array of at most 128 entries')
    require(level['rooms'] and level['spawns'], 'rooms and spawns are required')
    ids = set()

    def identify(item):
        value = item['id']
        require(isinstance(value,str) and re.fullmatch(r'[a-z][a-z0-9_]{0,31}',value), 'invalid entity id')
        require(value not in ids, 'duplicate id: '+value)
        ids.add(value)

    rooms = {}
    for r in level['rooms']:
        fields(r,('id','origin','size'))
        identify(r)
        vector(r['origin'])
        vector(r['size'],64,8192)
        require(all(v % 2 == 0 for v in r['size'][:2]), 'room horizontal dimensions must be even')
        rooms[r['id']] = r
    bounds = [room_bounds(r) for r in rooms.values()]
    for i,(a,A) in enumerate(bounds):
        require(all(-32752<=v<=32752 for v in a+A), 'room shell outside Quake world bounds')
        for b,B in bounds[i+1:]:
            require(not all(a[k]<B[k] and b[k]<A[k] for k in (0,1)), 'room interiors overlap horizontally')
            require(not all(a[k]-32<B[k] and b[k]-32<A[k] for k in (0,1)), 'rooms need 32 units between shells')
    passages = []
    for c in level['connections']:
        fields(c,('id','from','to','axis','at','width','height'),('door','transition'))
        identify(c)
        require(c['from'] in rooms and c['to'] in rooms and c['from'] != c['to'], 'connection names unknown/same room')
        require(c['axis'] in ('x','y'), 'connection axis must be x or y')
        number(c['at'])
        number(c['width'],1,8192)
        number(c['height'],1,8192)
        require(c['width'] >= max(64,rules['min_corridor_width']), 'corridor width below player/design clearance')
        require(c['width'] % 2 == 0, 'corridor width must be even')
        require(c['height'] >= max(80,rules['min_door_height']), 'door height below player/design clearance')
        require(type(c.get('door',False)) is bool, 'door must be boolean')
        require(c.get('transition') in (None,'stairs','ramp'), 'unknown transition')
        low,high,axis,start,end = passage(c,rooms)
        side = 1-axis
        require(high[axis]-low[axis]>=32, 'corridor requires a gap of at least 32 units')
        require(start==end or c.get('transition'), 'different floors require stairs or ramp')
        require(abs(end-start)/(high[axis]-low[axis])<=0.5, 'transition slope exceeds walking limit')
        if c.get('transition')=='stairs' and start!=end:
            require((high[axis]-low[axis])/math.ceil(abs(end-start)/16)>=32, 'stair tread below 32 units')
        for name in ('from','to'):
            a,A = room_bounds(rooms[c[name]])
            require(a[side]+16<=low[side] and high[side]<=A[side]-16 and high[2]<=A[2], 'connection opening outside room wall')
        for i,(a,A) in enumerate(bounds):
            require(not all(low[k]<A[k] and a[k]<high[k] for k in (0,1)), 'corridor intersects room interior')
        for a,A in passages:
            require(not all(low[k]-16<A[k] and a[k]-16<high[k] for k in (0,1)), 'corridors overlap or lack separating wall')
        passages.append((low,high))

    def surface(x,y):
        intervals = [(a[2],A[2]) for a,A in bounds if a[0]<=x<=A[0] and a[1]<=y<=A[1]]
        for c,(a,A) in zip(level['connections'],passages):
            if a[0]<=x<=A[0] and a[1]<=y<=A[1]:
                intervals.append((floor_at(c,rooms,(x,y)),A[2]))
        return (max(v[0] for v in intervals), min(v[1] for v in intervals)) if intervals else None

    obstacles, covers = [], []
    for item in level['cover']:
        fields(item,('id','origin','kit'),('size',))
        identify(item)
        vector(item['origin'])
        require(item['kit'] in ('low','tall'), 'unknown cover kit')
        size = item.get('size',[64,32,48 if item['kit']=='low' else 96])
        vector(size,16,2048)
        x,y,z = item['origin']
        w,d,h = size
        box = ([x-w/2,y-d/2,z],[x+w/2,y+d/2,z+h])
        obstacles.append(box)
        covers.append(box)
    for prop in level['props']:
        fields(prop,('id','model','origin','size','material','solid'))
        identify(prop)
        vector(prop['origin'])
        vector(prop['size'],1,2048)
        require(type(prop['solid']) is bool, 'solid must be boolean')
        require(prop['material'] in level['materials'], 'unknown prop material role')
        path = prop_asset(prop,assets)
        sources[prop['model']] = path
        a,b = [[v+sign*s/2 for v,s in zip(prop['origin'],prop['size'])] for sign in (-1,1)]
        require(any(all(lo[k]<=a[k] and b[k]<=hi[k] for k in range(3)) for lo,hi in bounds), 'prop outside room')
        if prop['solid']:
            obstacles.append((a,b))
    for a,A in obstacles:
        require(any(all(lo[k]<=a[k] and A[k]<=hi[k] for k in range(3)) for lo,hi in bounds), 'cover/prop outside room')
    for spawn in level['spawns']:
        fields(spawn,('team','origin','angle'))
        vector(spawn['origin'])
        number(spawn['angle'],0,359)
        require(spawn['team'] in ('ffa','red','blue'), 'unknown spawn team')
        x,y,z = spawn['origin']
        s = surface(x,y)
        require(s and abs(z-24-s[0])<0.01 and z+32<=s[1], 'spawn outside walkable world')
    for view in camera_specs(level.get('viewpoints',[])):
        x,y,z = view['origin']
        s = surface(x,y)
        require(s and s[0]<z<s[1] and not any(all(a[k]<=view['origin'][k]<=A[k] for k in range(3)) for a,A in obstacles),
                'viewpoint outside empty world space')
    pickups = {'weapon_gauntlet','weapon_machinegun','weapon_shotgun','weapon_grenadelauncher','weapon_rocketlauncher',
               'weapon_lightning','weapon_railgun','weapon_plasmagun','weapon_bfg','ammo_bullets','ammo_shells',
               'ammo_grenades','ammo_rockets','ammo_lightning','ammo_slugs','ammo_cells','ammo_bfg',
               'item_health','item_health_small','item_health_large','item_health_mega','item_armor_shard',
               'item_armor_combat','item_armor_body','team_CTF_redflag','team_CTF_blueflag'}
    for item in level['pickups']:
        fields(item,('classname','origin'))
        vector(item['origin'])
        require(item['classname'] in pickups, 'unsupported pickup classname')
        x,y,z = item['origin']
        s = surface(x,y)
        require(s and s[0]<=z<=s[1]-16, 'pickup outside world')
    lighting = level['lighting']
    fields(lighting,('ambient','lights'),('sun','directional'))
    require(isinstance(lighting.get('directional',False),bool), 'directional must be a boolean')
    number(lighting['ambient'],0,255)
    require(isinstance(lighting['lights'],list) and len(lighting['lights'])<=128, 'too many lights')
    for light in lighting['lights']:
        fields(light,('id','origin','color','intensity'))
        identify(light)
        vector(light['origin'])
        vector(light['color'],0,1,False)
        number(light['intensity'],1,100000)
        x,y,z = light['origin']
        s = surface(x,y)
        require(s and s[0]<z<s[1], 'light outside world')
    if 'sun' in lighting:
        sun = lighting['sun']
        fields(sun,('direction','color','intensity'))
        vector(sun['direction'],integer=False)
        vector(sun['color'],0,1,False)
        number(sun['intensity'],1,100000)
        require(sum(v*v for v in sun['direction'])>0, 'sun direction is zero')

    # ponytail: whole-layout diagonal conservatively bounds all sightlines; use visibility
    # tracing if authors need a larger winding level under a shorter sightline limit.
    lower = [min(a[k] for a,A in bounds) for k in range(3)]
    upper = [max(A[k] for a,A in bounds) for k in range(3)]
    sightline = math.dist(lower,upper)
    require(sightline<=rules['max_sightline'], 'sightline upper bound exceeds design limit')
    require((upper[0]-lower[0])*(upper[1]-lower[1])<=16*16*262144, 'navigation grid exceeds 262144 cells')

    def walkable(x,y):
        surfaces = [surface(x+dx,y+dy) for dx in (-15,0,15) for dy in (-15,0,15)]
        if any(s is None for s in surfaces):
            return None
        floor = max(s[0] for s in surfaces)
        if max(s[0] for s in surfaces)-min(s[0] for s in surfaces)>18 or min(s[1] for s in surfaces)-floor<56:
            return None
        if any(x+15>a[0] and x-15<A[0] and y+15>a[1] and y-15<A[1] and floor<A[2] and floor+56>a[2] for a,A in obstacles):
            return None
        return floor

    cells = {}
    for x in range(math.ceil(lower[0]/16)*16,math.floor(upper[0]/16)*16+1,16):
        for y in range(math.ceil(lower[1]/16)*16,math.floor(upper[1]/16)*16+1,16):
            z = walkable(x,y)
            if z is not None:
                cells[x,y] = z
    spawn_cells = []
    for spawn in level['spawns']:
        x,y,z = spawn['origin']
        require(walkable(x,y) is not None, 'spawn outside player clearance')
        near = sorted((math.hypot(X-x,Y-y),(X,Y)) for X,Y in cells if abs(X-x)<=16 and abs(Y-y)<=16 and abs(cells[X,Y]-(z-24))<=18)
        require(near, 'unreachable spawn: no navigation cell')
        spawn_cells.append(near[0][1])
    visited = {spawn_cells[0]}
    queue = deque(visited)
    while queue:
        p = queue.popleft()
        for dx,dy in ((16,0),(-16,0),(0,16),(0,-16)):
            q = (p[0]+dx,p[1]+dy)
            middle = walkable((p[0]+q[0])/2,(p[1]+q[1])/2) if q in cells else None
            if q in cells and q not in visited and abs(cells[q]-cells[p])<=18 and middle is not None:
                visited.add(q)
                queue.append(q)
    require(all(c in visited for c in spawn_cells), 'unreachable spawn: disconnected walkable regions')
    require(covers, 'cover gap: at least one cover piece is required')
    cover_gap = 0
    for x,y in visited:
        distance = min(math.hypot(max(a[0]-x,0,x-A[0]),max(a[1]-y,0,y-A[1])) for a,A in covers)
        cover_gap = max(cover_gap,distance+math.sqrt(2)*8)
    require(cover_gap<=rules['max_cover_gap'], 'cover gap exceeds design limit')
    return sources, {'rooms':len(rooms),'connections':len(passages),'reachable_spawns':len(spawn_cells),
                     'navigation_cells':len(visited),'sightline_upper_bound':round(sightline,3),'max_cover_gap':round(cover_gap,3)}
