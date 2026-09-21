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


def prism(poly, low, high, material, slope=None):
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
                         f' {material} 0 0 0 1 1 0 0 0\n' for plane in planes)+'}\n'


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
    add('boundary_floor',area,floor-16,floor,'floor')
    add('boundary_ceiling',area,ceiling,ceiling+16,'sky')
    add('boundary_wall',area.difference(area.buffer(-16,join_style='mitre')),floor,ceiling,'wall')
    ids = set()
    for item in level['shapes']:
        identity = item['id']
        require(identity not in ids, 'duplicate shape id: '+identity)
        ids.add(identity)
        p = footprint(item['shape'])
        z,Z = item['base'],item['base']+item['height']
        require(area.buffer(-16,join_style='mitre').covers(p) and floor<=z<Z<=ceiling,
                'shape outside playable boundary: '+identity)
        material = item.get('material','wall' if item['kind']=='building' else 'cover')
        require(material in level['materials'], 'unknown material role: '+material)
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
        for opening in item.get('openings',[]):
            bottom,top = z+opening['sill'],z+opening['sill']+opening['height']
            require(z<=bottom<top<=Z-16, 'opening height outside wall')
            cutter = opening_polygon(p,opening,thickness)
            cuts.update((bottom,top))
            openings.append((bottom,top,cutter))
        for low,high in zip(sorted(cuts),sorted(cuts)[1:]):
            section = walls
            for bottom,top,cutter in openings:
                if bottom<=low and high<=top:
                    section = section.difference(cutter)
            add(identity,section,low,high,material)
        for i in range(1,floors+1):
            top = z+item['height']*i/floors
            add(identity,inner,top-16,top,'trim')
    require(len(result)<=4096, 'level piece budget exceeded')
    return area,result


def generate(level):
    _,records = pieces(level)
    world = [prism(poly,record['low'],record['high'],level['materials'][record['material']],record['slope'])
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
        entities.append(entity({'classname':'misc_model','model':prop['model'],'origin':vector(prop['origin']),
                                '_remap':'*;textures/'+level['materials'][prop['material']]}))
    worldspawn = {'classname':'worldspawn','message':level['name'],'_minlight':level['lighting']['ambient']}
    if level['lighting'].get('directional',False):
        worldspawn['_aftershock_deluxe'] = 1
    return entity(worldspawn,world)+''.join(entities)


def navigation(level, records):
    """Clearance layers retain separate floors at the same XY, with 18-unit steps."""
    from collections import deque
    import numpy as np
    area = polygon(level['boundary']['polygon'],level['boundary'].get('holes',[]))
    x,y,X,Y = area.bounds
    require((X-x)*(Y-y)<=16*16*262144, 'navigation grid exceeds 262144 cells')
    xy = np.array([(a,b) for a in range(math.ceil(x/16)*16,math.floor(X/16)*16+1,16)
                   for b in range(math.ceil(y/16)*16,math.floor(Y/16)*16+1,16)])
    points = shapely.points(xy)
    layers,cells = {},{}
    for z in sorted({record['high'] for record in records}):
        if z>=level['boundary']['ceiling']:
            continue
        support = shapely.union_all([r['polygon'] for r in records if r['high']==z])
        obstacles = shapely.union_all([r['polygon'] for r in records if r['low']<z+56 and r['high']>z+1e-5])
        # Square player hull: bevel-free mitred buffers conservatively keep corners clear.
        free = support.buffer(-15,join_style='mitre').difference(obstacles.buffer(15,join_style='mitre'))
        layers[z] = free
        for a,b in xy[shapely.covers(free,points)]:
            cells.setdefault((int(a),int(b)),[]).append(z)
    require(sum(map(len,cells.values()))<=262144, 'navigation cell budget exceeded')
    starts = []
    for spawn in level['spawns']:
        a,b,z = spawn['origin']
        floor = z-24
        require(any(abs(floor-h)<.01 and region.covers(Point(a,b)) for h,region in layers.items()),
                'spawn outside player clearance')
        near = [(math.hypot(A-a,B-b),(A,B,h)) for A,B in cells if abs(A-a)<=16 and abs(B-b)<=16
                for h in cells[A,B] if abs(h-floor)<18]
        require(near, 'spawn has no navigation cell')
        starts.append(min(near)[1])
    require(starts, 'at least one spawn required')
    seen = {starts[0]}
    queue = deque(seen)
    while queue:
        a,b,z = queue.popleft()
        for dx,dy in ((16,0),(-16,0),(0,16),(0,-16)):
            target = (a+dx,b+dy)
            for h in cells.get(target,[]):
                node = (*target,h)
                if abs(h-z)>18 or node in seen:
                    continue
                # The swept center segment must remain in the union of adjacent layers.
                if not layers[z].union(layers[h]).covers(LineString([(a,b),target])):
                    continue
                seen.add(node)
                queue.append(node)
    require(all(p in seen for p in starts), 'unreachable spawn: disconnected walkable regions')
    cover = shapely.union_all([r['polygon'] for r in records if not r['id'].startswith('boundary_')])
    require(not cover.is_empty, 'cover gap: at least one shape is required')
    gap = max(float(v) for v in shapely.distance(cover,shapely.points([(a,b) for a,b,_ in seen])))+math.sqrt(2)*8
    require(gap<=level['rules']['max_cover_gap'], 'cover gap exceeds design limit')
    diagonal = math.sqrt((X-x)**2+(Y-y)**2+(level['boundary']['ceiling']-level['boundary']['floor'])**2)
    require(diagonal<=level['rules']['max_sightline'], 'sightline upper bound exceeds design limit')
    return dict(reachable_spawns=len(starts),navigation_cells=len(seen),max_cover_gap=round(gap,3),
                sightline_upper_bound=round(diagonal,3))


def validate(level, assets):
    from validate import material_sources
    sources = material_sources(level['materials'],assets)
    area,records = pieces(level)
    require(not level['props'], 'v2 mesh props require theme assembly before validation')
    report = navigation(level,records)
    report.update(shapes=len(level['shapes']),playable_area=round(area.area,3),brushes=sum(len(convex_parts(r['polygon'])) for r in records))
    return sources,report
