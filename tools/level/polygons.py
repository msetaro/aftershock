"""Version-2 planar CSG and convex MAP brushes; v1 retains its original generator."""
import math

import shapely
from shapely import Polygon, LineString, Point, box
from shapely.geometry.polygon import orient

from geometry import entity, vector


def require(condition, message):
    if not condition:
        raise ValueError(message)


def polygon(points, holes=()):
    result = Polygon(points, holes)
    require(result.is_valid and not result.is_empty and result.area >= 1,
            'polygon must be simple, nonempty and have at least one square unit of area')
    require(len(result.exterior.coords) <= 257 and sum(len(r.coords) for r in result.interiors) <= 1024,
            'polygon vertex budget exceeded')
    require(all(math.isfinite(v) and abs(v)<=32000 for v in shapely.get_coordinates(result).flat),
            'polygon coordinates must be finite and within world bounds')
    return orient(result, sign=1)


def footprint(shape):
    if 'polygon' in shape:
        return polygon(shape['polygon'], shape.get('holes', []))
    if 'rectangle' in shape:
        r = shape['rectangle']
        x,y = r['center']
        w,d = r['size']
        require(w>0 and d>0, 'rectangle dimensions must be positive')
        angle = math.radians(r.get('angle',0))
        c,s = math.cos(angle),math.sin(angle)
        return polygon([[x+u*c-v*s,y+u*s+v*c] for u,v in ((-w/2,-d/2),(w/2,-d/2),(w/2,d/2),(-w/2,d/2))])
    if 'circle' in shape or 'arc' in shape:
        arc = 'arc' in shape
        r = shape['arc' if arc else 'circle']
        radius,tolerance = r['radius'],r['tolerance']
        require(radius>0 and 0<tolerance<radius, 'curve tolerance must be positive and smaller than radius')
        start,end = (r['start'],r['end']) if arc else (0,360)
        require(0<end-start<=360, 'arc sweep must be in (0,360] degrees')
        count = max(8,math.ceil(math.radians(end-start)/(2*math.acos(1-tolerance/radius))))
        require(count<=256, 'curve tolerance needs more than 256 segments')
        x,y = r['center']
        points = [(x+radius*math.cos(math.radians(start+(end-start)*i/count)),
                   y+radius*math.sin(math.radians(start+(end-start)*i/count))) for i in range(count+int(arc))]
        result = LineString(points).buffer(r['thickness']/2,cap_style='flat',join_style='mitre') if arc else Polygon(points)
    else:
        r = shape['path']
        require(r['thickness']>0, 'path thickness must be positive')
        result = LineString(r['points']).buffer(r['thickness']/2,cap_style='flat',join_style='mitre')
    require(result.geom_type=='Polygon', 'shape must produce one polygon')
    return polygon(list(result.exterior.coords)[:-1],[list(r.coords)[:-1] for r in result.interiors])


def components(shape):
    if shape.is_empty:
        return []
    return [shape] if shape.geom_type=='Polygon' else [p for p in shape.geoms if p.geom_type=='Polygon' and p.area>1e-6]


def convex_parts(shape):
    """Constrained triangulation preserves holes; stable order fixes brush numbering."""
    pieces = []
    for p in components(shape):
        if not p.interiors and abs(p.convex_hull.area-p.area)<1e-6:
            pieces.append(p)
        else:
            pieces.extend(shapely.constrained_delaunay_triangles(p).geoms)
    return sorted((orient(p,sign=1) for p in pieces if p.area>1e-6),key=lambda p:(p.bounds,p.wkb_hex))


def prism(poly, low, high, material, slope=None, texture_scale=1):
    points = list(poly.exterior.coords)[:-1]
    x,y = points[0]
    planes = [((x,y,low),(x+16,y,low),(x,y+16,low))]
    if slope is None:
        planes.append(((x,y,high),(x,y+16,high),(x+16,y,high)))
    else:
        start,end,width,z,Z = slope
        dx,dy = end[0]-start[0],end[1]-start[1]
        length = math.hypot(dx,dy)
        planes.append(((start[0],start[1],z),(start[0]-dy/length*width,start[1]+dx/length*width,z),
                       (end[0],end[1],Z)))
    for a,b in zip(points,points[1:]+points[:1]):
        planes.append(((a[0],a[1],low),(a[0],a[1],low+16),(b[0],b[1],low)))
    return '{\n'+''.join(' '.join('( '+vector(point)+' )' for point in plane)+
                         f' {material} 0 0 0 {texture_scale:.9g} {texture_scale:.9g} 0 0 0\n' for plane in planes)+'}\n'


def opening_polygon(poly, opening, thickness):
    points = list(poly.exterior.coords)
    edge = opening['edge']
    require(0<=edge<len(points)-1, 'opening edge outside building polygon')
    a,b = points[edge:edge+2]
    length = math.dist(a,b)
    at,width = opening['at'],opening['width']
    require(width>=32 and width/2+thickness<=at<=length-width/2-thickness, 'opening must fit wall with corner clearance')
    dx,dy = (b[0]-a[0])/length,(b[1]-a[1])/length
    return Polygon([(a[0]+dx*u-dy*v,a[1]+dy*u+dx*v)
                    for u,v in ((at-width/2,-thickness),(at+width/2,-thickness),
                                (at+width/2,thickness*2),(at-width/2,thickness*2))])


def stair_sections(start,end,width,z,Z,slab=False):
    count = math.ceil(abs(Z-z)/16)
    require(count and math.dist(start,end)/count>=32, 'stair tread below 32 units')
    result = []
    for i in range(count):
        points = [[start[k]+(end[k]-start[k])*f/count for k in (0,1)] for f in (i,i+1)]
        section = LineString(points).buffer(width/2,cap_style='flat',join_style='mitre')
        top = z+(Z-z)*(i if z>Z else i+1)/count
        result.append((section,top-16 if slab else min(z,Z)-16,top))
    return result


def building_stairs(inner,rise,width):
    corners = list(orient(inner.minimum_rotated_rectangle,sign=1).exterior.coords)
    a,b = max(zip(corners,corners[1:]),key=lambda pair:(round(math.dist(*pair),6),pair[1][0]-pair[0][0],pair[1][1]-pair[0][1]))
    length = math.dist(a,b)
    dx,dy = (b[0]-a[0])/length,(b[1]-a[1])/length
    center = inner.centroid
    run = math.ceil(math.ceil(rise/16)/2)*32
    def point(u,v):
        return [center.x+dx*(u-width/2)-dy*v,center.y+dy*(u-width/2)+dx*v]
    well = Polygon([point(u,v) for u,v in ((-run/2,-width),(run/2+width,-width),(run/2+width,width),(-run/2,width))])
    require(inner.buffer(-1).covers(well), 'building too small for interior stairs; enlarge footprint or lower storey height')
    first = (point(-run/2,-width/2),point(run/2,-width/2))
    second = (point(run/2,width/2),point(-run/2,width/2))
    landing = Polygon([point(u,v) for u,v in ((run/2,-width),(run/2+width,-width),(run/2+width,width),(run/2,width))])
    return well,first,second,landing


def ruled_openings(item,p,floors):
    result = list(item.get('openings',[]))
    points = list(p.exterior.coords)
    for rule in item.get('opening_rules',[]):
        target = rule['face_point']
        for edge,(a,b) in enumerate(zip(points,points[1:])):
            dx,dy = b[0]-a[0],b[1]-a[1]
            if (target[0]-(a[0]+b[0])/2)*dy-(target[1]-(a[1]+b[1])/2)*dx<=0:
                continue
            length = math.dist(a,b)
            count = max(1,math.floor(length/rule['spacing']))
            for floor in rule.get('floors',list(range(floors))):
                require(0<=floor<floors, 'opening rule names an unavailable floor')
                for i in range(count):
                    result.append(dict(edge=edge,at=length*(i+.5)/count,width=rule['width'],
                                       sill=rule['sill']+floor*item['height']/floors,height=rule['height']))
    require(len(result)<=256, 'building opening budget exceeded')
    return result


def prop_bounds(prop):
    x,y,z = prop['origin']
    a,b,c = prop.get('bounds_center',[0,0,0])
    w,d,h = prop['size']
    angle = math.radians(prop.get('angle',0))
    cosine,sine = math.cos(angle),math.sin(angle)
    shape = footprint(dict(rectangle=dict(center=[x+a*cosine-b*sine,y+a*sine+b*cosine],
                                          size=[w,d],angle=prop.get('angle',0))))
    return shape,z+c-h/2,z+c+h/2


def pieces(level):
    boundary = level['boundary']
    area = polygon(boundary['polygon'],boundary.get('holes',[]))
    floor,ceiling = boundary['floor'],boundary['ceiling']
    require(ceiling-floor>=128, 'boundary ceiling must provide player clearance')
    require(area.area<=16*16*262144, 'navigation area budget exceeded')
    # Records retain shape IDs so note edits can be compared without renumbering.
    result = []
    def add(identity,p,z,Z,material,slope=None):
        if not p.is_empty and Z>z:
            require(-32752<=z<Z<=32752, 'brush height outside world bounds')
            result.append(dict(id=identity,polygon=p,low=z,high=Z,material=material,slope=slope))
    authored = [(item,footprint(item['shape'])) for item in level['shapes']]
    sunken = [(item,p) for item,p in authored if item['kind']=='platform' and item['base']+item['height']<floor]
    floor_area = area.difference(shapely.union_all([p for _,p in sunken]))
    add('boundary_floor',floor_area,floor-16,floor,'floor')
    for item,p in sunken:
        retaining = p.buffer(16,join_style='mitre').difference(p)
        add(item['id'],retaining,item['base'],floor,'wall')
    add('boundary_ceiling',area,ceiling,ceiling+16,'sky')
    add('boundary_wall',area.difference(area.buffer(-16,join_style='mitre')),floor,ceiling,'wall')
    ids = set()
    for item,p in authored:
        identity = item['id']
        require(identity not in ids and not identity.startswith('boundary_'), 'duplicate/reserved shape id: '+identity)
        ids.add(identity)
        z,Z = item['base'],item['base']+item['height']
        require(area.buffer(-16,join_style='mitre').covers(p) and min([floor]+[r['base'] for r,_ in sunken])<=z<Z<=ceiling,
                'shape outside playable boundary: '+identity)
        material = item.get('material','wall' if item['kind']=='building' else 'cover')
        require(material in level['materials'], 'unknown material role: '+material)
        if item.get('bullet_solid') is False:
            require(item['kind']=='wall', 'bullet transparency is only supported for wall/fence shapes')
            material = 'level/fence'
        if item['kind']=='transition':
            path = item['shape'].get('path',{})
            require(len(path.get('points',[]))==2, 'transition needs a two-point path')
            start,end = path['points']
            width = path['thickness']
            length = math.dist(start,end)
            require(width>=max(64,level['rules']['min_corridor_width']), 'transition width below player/design clearance')
            require(item['height']/length<=.5, 'transition slope exceeds walking limit')
            a,b = (Z,z) if item.get('descending',False) else (z,Z)
            if item['transition']=='ramp':
                add(identity,p,z-16,Z,material,(start,end,width,a,b))
            else:
                for section,low,high in stair_sections(start,end,width,a,b):
                    add(identity,section,low,high,material)
            continue
        if item['kind']!='building':
            add(identity,p,z,Z,material)
            continue
        thickness = item.get('wall_thickness',16)
        require(4<=thickness<=128, 'wall thickness outside 4..128')
        inner = p.buffer(-thickness,join_style='mitre')
        require(not inner.is_empty and inner.geom_type=='Polygon', 'building walls leave no connected interior')
        walls = p.difference(inner)
        floors = item.get('floors',1)
        require(type(floors) is int and 1<=floors<=8 and item['height']/floors>=96,
                'building floors need at least 96 units per floor')
        openings = []
        cuts = {z,Z}
        for opening in ruled_openings(item,p,floors):
            bottom,top = z+opening['sill'],z+opening['sill']+opening['height']
            require(z<=bottom<top<=Z-16, 'opening height outside wall')
            if abs(opening['sill'] % (item['height']/floors))<1e-5:
                require(opening['width']>=max(64,level['rules']['min_corridor_width']) and
                        opening['height']>=max(80,level['rules']['min_door_height']), 'door below player/design clearance')
            cutter = opening_polygon(p,opening,thickness)
            cuts.update((bottom,top))
            openings.append((bottom,top,cutter))
        for low,high in zip(sorted(cuts),sorted(cuts)[1:]):
            section = walls
            for bottom,top,cutter in openings:
                if bottom<=low and high<=top:
                    section = section.difference(cutter)
            add(identity,section,low,high,material)
        stair_count = floors if item.get('roof_access',False) else floors-1
        well = Polygon()
        if stair_count:
            rise = item['height']/floors
            width = max(64,level['rules']['min_corridor_width'])
            well,first,second,landing = building_stairs(inner,rise,width)
            for i in range(stair_count):
                low,high = z+i*rise,z+(i+1)*rise
                middle = low+rise/2
                for start,end,a,b in ((*first,low,middle),(*second,middle,high)):
                    for section,bottom,top in stair_sections(start,end,width,a,b,slab=True):
                        add(identity,section,bottom,top,'trim')
                add(identity,landing,middle-16,middle,'trim')
        for i in range(1,floors+1):
            top = z+item['height']*i/floors
            slab = inner.difference(well) if i<=stair_count else inner
            add(identity,slab,top-16,top,'trim')
    for prop in level['props']:
        require(prop['id'] not in ids and not prop['id'].startswith('boundary_'), 'duplicate/reserved shape/prop id: '+prop['id'])
        ids.add(prop['id'])
        p,z,Z = prop_bounds(prop)
        require(area.buffer(-16,join_style='mitre').covers(p) and min([floor]+[r['base'] for r,_ in sunken])<=z<Z<=ceiling,
                'prop outside playable boundary: '+prop['id'])
        if prop['solid']:
            add(prop['id'],p,z,Z,'level/playerclip')
    require(len(result)<=4096, 'level piece budget exceeded')
    return area,result


def surface_material(level,record):
    material = record['material']
    if record['id'].startswith('boundary_') or material.startswith('level/'):
        return level['materials'].get(material,material)
    return 's/'+record['id']+'/'+material


def surface_shaders(level,cooked=()):
    _,records = pieces(level)
    aliases = sorted({(surface_material(level,r),level['materials'].get(r['material'],r['material']))
                      for r in records if not r['id'].startswith('boundary_') and not r['material'].startswith('level/') and r['material'] not in cooked})
    return ''.join(f'textures/{alias}\n{{\n    qer_editorimage textures/{source}\n'
                   '    {\n        map $lightmap\n        rgbGen identity\n    }\n'
                   f'    {{\n        map textures/{source}\n        blendFunc filter\n        rgbGen identity\n    }}\n}}\n'
                   for alias,source in aliases)


def generate(level):
    _,records = pieces(level)
    world = ['// shape '+record['id']+'\n'+prism(poly,record['low'],record['high'],surface_material(level,record),record['slope'],level.get('texture_scale',{}).get(record['material'],1))
             for record in records for poly in convex_parts(record['polygon'])]
    require(len(world)<=8192, 'convex brush budget exceeded')
    entities = []
    for spawn in level['spawns']:
        classes = ['info_player_deathmatch'] if spawn['team']=='ffa' else [f"team_CTF_{spawn['team']}player",f"team_CTF_{spawn['team']}spawn"]
        entities.extend(entity({'classname':name,'origin':vector(spawn['origin']),'angle':spawn['angle']}) for name in classes)
    for pickup in level['pickups']:
        entities.append(entity({'classname':pickup['classname'],'origin':vector(pickup['origin'])}))
    for light in level['lighting']['lights']:
        entities.append(entity({'classname':'light','targetname':light['id'],'origin':vector(light['origin']),
                                '_color':vector(light['color']),'light':light['intensity']}))
    for prop in level['props']:
        entities.append(entity({'classname':'misc_model','model':prop['model'],'origin':vector(prop['origin']),'angle':prop.get('angle',0),
                                '_remap':'*;textures/'+level['materials'][prop['material']]}))
    worldspawn = {'classname':'worldspawn','message':level['name'],'_minlight':level['lighting']['ambient']}
    if level['lighting'].get('directional',False):
        worldspawn['_aftershock_deluxe'] = 1
    return entity(worldspawn,world)+''.join(entities)


def surface_height(record,x,y):
    if record['slope'] is None:
        return record['high']
    start,end,_,z,Z = record['slope']
    dx,dy = end[0]-start[0],end[1]-start[1]
    fraction = ((x-start[0])*dx+(y-start[1])*dy)/(dx*dx+dy*dy)
    return z+(Z-z)*max(0,min(1,fraction))


def navigation(level, records, routes=None):
    """Sample a square player hull at each surface; separate floors share XY nodes."""
    from collections import deque
    area = polygon(level['boundary']['polygon'],level['boundary'].get('holes',[]))
    x,y,X,Y = area.bounds
    require((X-x)*(Y-y)<=16*16*262144, 'navigation grid exceeds 262144 cells')
    tree = shapely.STRtree([r['polygon'] for r in records])
    def walkable(a,b):
        hull = box(a-15,b-15,a+15,b+15)
        nearby = [records[i] for i in tree.query(hull,predicate='intersects')]
        candidates = {surface_height(r,a,b) for r in nearby if r['polygon'].covers(Point(a,b))}
        result = []
        for candidate in sorted(candidates):
            heights = []
            for dx,dy in ((0,0),(-15,-15),(-15,15),(15,-15),(15,15),(-15,0),(15,0),(0,-15),(0,15)):
                point = Point(a+dx,b+dy)
                surfaces = [surface_height(r,a+dx,b+dy) for r in nearby if r['polygon'].covers(point)]
                heights.append(max((h for h in surfaces if h<=candidate+18),default=-math.inf))
            top = max(heights)
            if top-min(heights)>18 or top>=level['boundary']['ceiling']:
                continue
            blocked = False
            for r in nearby:
                intersection = hull.intersection(r['polygon'])
                if intersection.is_empty or intersection.area<1e-6:
                    continue
                high = max(surface_height(r,u,v) for u,v in shapely.get_coordinates(intersection))
                if r['low']<top+56 and high>top+1e-5:
                    blocked = True
                    break
            if not blocked and not any(abs(top-h)<1e-5 for h in result):
                result.append(top)
        return result
    cells = {}
    for a in range(math.ceil(x/16)*16,math.floor(X/16)*16+1,16):
        for b in range(math.ceil(y/16)*16,math.floor(Y/16)*16+1,16):
            heights = walkable(a,b)
            if heights:
                cells[a,b] = heights
    require(sum(map(len,cells.values()))<=262144, 'navigation cell budget exceeded')
    starts = []
    for spawn in level['spawns']:
        a,b,z = spawn['origin']
        require(any(abs(z-24-h)<.01 for h in walkable(a,b)), 'spawn outside player clearance')
        near = [(math.hypot(A-a,B-b),(A,B,h)) for A,B in cells if abs(A-a)<=16 and abs(B-b)<=16
                for h in cells[A,B] if abs(h-(z-24))<=18]
        require(near, 'spawn has no navigation cell')
        starts.append(min(near)[1])
    require(starts, 'at least one spawn required')
    seen = {starts[0]}
    queue = deque(seen)
    parents = {starts[0]:None} if routes is not None else None
    while queue:
        a,b,z = queue.popleft()
        for dx,dy in ((16,0),(-16,0),(0,16),(0,-16)):
            target = (a+dx,b+dy)
            for h in cells.get(target,[]):
                node = (*target,h)
                if abs(h-z)>18 or node in seen:
                    continue
                if not any(abs(m-z)<=18 and abs(m-h)<=18 for m in walkable(a+dx/2,b+dy/2)):
                    continue
                seen.add(node)
                if parents is not None:
                    parents[node] = (a,b,z)
                queue.append(node)
    require(all(p in seen for p in starts), 'unreachable spawn: disconnected walkable regions')
    if routes is not None:
        for target in starts:
            route = []
            while target is not None:
                route.append(target)
                target = parents[target]
            routes.append(list(reversed(route)))
    cover = shapely.union_all([r['polygon'] for r in records if not r['id'].startswith('boundary_')])
    require(not cover.is_empty, 'cover gap: at least one shape is required')
    gap = max(float(v) for v in shapely.distance(cover,shapely.points([(a,b) for a,b,_ in seen])))+math.sqrt(2)*8
    require(gap<=level['rules']['max_cover_gap'], 'cover gap exceeds design limit')
    diagonal = math.sqrt((X-x)**2+(Y-y)**2+(level['boundary']['ceiling']-level['boundary']['floor'])**2)
    require(diagonal<=level['rules']['max_sightline'], 'sightline upper bound exceeds design limit')
    return dict(reachable_spawns=len(starts),navigation_cells=len(seen),max_cover_gap=round(gap,3),
                sightline_upper_bound=round(diagonal,3))


def validate(level, assets):
    from validate import material_sources, prop_asset
    from tools.agent.formats import validate as validate_format
    validate_format('level',level,'<level>')
    sources = material_sources(level['materials'],assets)
    area,records = pieces(level)
    for prop in level['props']:
        require(prop['material'] in level['materials'], 'unknown prop material role')
        sources[prop['model']] = prop_asset(prop,assets)
    report = navigation(level,records)
    report.update(shapes=len(level['shapes']),playable_area=round(area.area,3),brushes=sum(len(convex_parts(r['polygon'])) for r in records))
    return sources,report
