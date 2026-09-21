"""Axis-aligned parametric MAP pieces; coordinates are Quake units."""
import math


def vector(values):
    return ' '.join(format(v, '.9g') for v in values)


def entity(values, brushes=()):
    return '{\n' + ''.join(f'"{key}" "{value}"\n' for key, value in values.items()) + ''.join(brushes) + '}\n'


def brush(low, high, material, ramp=None):
    x, y, z = low
    X, Y, Z = high
    planes = [((x,y,z),(x,y+16,z),(x,y,z+16)),
              ((X,y,z),(X,y,z+16),(X,y+16,z)),
              ((x,y,z),(x,y,z+16),(x+16,y,z)),
              ((x,Y,z),(x+16,Y,z),(x,Y,z+16)),
              ((x,y,z),(x+16,y,z),(x,y+16,z)),
              ((x,y,Z),(x,y+16,Z),(x+16,y,Z))]
    if ramp:
        axis, start, end = ramp
        planes[-1] = (((x,y,start),(x,Y,start),(X,y,end)) if axis == 0 else
                      ((x,y,start),(x,Y,end),(X,y,start)))
    return '{\n' + ''.join(' '.join('( ' + vector(point) + ' )' for point in plane) +
                            f' {material} 0 0 0 1 1 0 0 0\n' for plane in planes) + '}\n'


def room_bounds(room):
    x, y, z = room['origin']
    w, d, h = room['size']
    return [x-w/2, y-d/2, z], [x+w/2, y+d/2, z+h]


def passage(connection, rooms):
    axis = 'xy'.index(connection['axis'])
    side = 1-axis
    a, A = room_bounds(rooms[connection['from']])
    b, B = room_bounds(rooms[connection['to']])
    low, high = [0,0,min(a[2],b[2])], [0,0,max(a[2],b[2])+connection['height']]
    low[axis], high[axis] = A[axis], b[axis]
    low[side], high[side] = connection['at']-connection['width']/2, connection['at']+connection['width']/2
    return low, high, axis, a[2], b[2]


def floor_at(connection, rooms, point):
    low, high, axis, start, end = passage(connection, rooms)
    fraction = max(0, min(1, (point[axis]-low[axis])/(high[axis]-low[axis])))
    if connection.get('transition') == 'stairs' and start != end:
        count = math.ceil(abs(end-start)/16)
        fraction = min(count, math.floor(fraction*count)+1)/count
    return start + (end-start)*fraction


def generate(level):
    if level.get('version')==2:
        from polygons import generate as generate_polygons
        return generate_polygons(level)
    materials = level['materials']
    rooms = {r['id']: r for r in level['rooms']}
    world, entities = [], []
    for room in rooms.values():
        low, high = room_bounds(room)
        world.append(brush([low[0]-16,low[1]-16,low[2]-16], [high[0]+16,high[1]+16,low[2]], materials['floor']))
        world.append(brush([low[0]-16,low[1]-16,high[2]], [high[0]+16,high[1]+16,high[2]+16], materials['sky']))
        for axis in (0,1):
            side = 1-axis
            for end in (0,1):
                position = (low,high)[end][axis]
                holes = []
                for c in level['connections']:
                    if 'xy'.index(c['axis']) != axis or c['from' if end else 'to'] != room['id']:
                        continue
                    p, P, _, start, finish = passage(c, rooms)
                    holes.append((p[side],P[side],low[2],P[2]))
                cuts = sorted({low[side]-16,high[side]+16,*(v for hole in holes for v in hole[:2])})
                heights = sorted({low[2],high[2],*(v for hole in holes for v in hole[2:])})
                for s,t in zip(cuts,cuts[1:]):
                    for z,Z in zip(heights,heights[1:]):
                        if any(h[0] <= s and t <= h[1] and h[2] <= z and Z <= h[3] for h in holes):
                            continue
                        a,b = [0,0,z],[0,0,Z]
                        a[axis],b[axis] = (position,position+16) if end else (position-16,position)
                        a[side],b[side] = s,t
                        world.append(brush(a,b,materials['wall']))
    for c in level['connections']:
        low,high,axis,start,end = passage(c,rooms)
        side = 1-axis
        a,b = low.copy(),high.copy()
        a[2],b[2] = min(start,end)-16,max(start,end)
        if c.get('transition') == 'stairs' and start != end:
            count = math.ceil(abs(end-start)/16)
            for i in range(count):
                s,t = a.copy(),b.copy()
                s[axis] = low[axis]+(high[axis]-low[axis])*i/count
                t[axis] = low[axis]+(high[axis]-low[axis])*(i+1)/count
                t[2] = start+(end-start)*(i+1)/count
                world.append(brush(s,t,materials['floor']))
        else:
            world.append(brush(a,b,materials['floor'], (axis,start,end) if start != end else None))
        a,b = low.copy(),high.copy()
        a[2],b[2] = high[2],high[2]+16
        a[side]-=16
        b[side]+=16
        world.append(brush(a,b,materials['trim']))
        for end_side in (0,1):
            a,b = low.copy(),high.copy()
            a[side],b[side] = (high[side],high[side]+16) if end_side else (low[side]-16,low[side])
            a[2] = min(start,end)-16
            a[axis] += 16
            b[axis] -= 16
            if a[axis] < b[axis]:
                world.append(brush(a,b,materials['wall']))
        if c.get('door'):
            a,b = low.copy(),high.copy()
            a[2] = start
            b[axis] = a[axis]+8
            entities.append(entity({'classname':'func_door','angle':'-1','speed':'200','wait':'2','lip':'4'},
                                   [brush(a,b,materials['trim'])]))
    for cover in level['cover']:
        x,y,z = cover['origin']
        w,d,h = cover.get('size', [64,32,48 if cover['kit']=='low' else 96])
        world.append(brush([x-w/2,y-d/2,z],[x+w/2,y+d/2,z+h],materials['cover']))
    for prop in level['props']:
        entities.append(entity({'classname':'misc_model','model':prop['model'],'origin':vector(prop['origin']),
                                '_remap': '*;textures/'+materials[prop['material']]}))
        if prop['solid']:
            a = [v-s/2 for v,s in zip(prop['origin'],prop['size'])]
            b = [v+s/2 for v,s in zip(prop['origin'],prop['size'])]
            world.append(brush(a,b,'level/playerclip'))
    for spawn in level['spawns']:
        team = spawn['team']
        classes = ['info_player_deathmatch'] if team=='ffa' else [f'team_CTF_{team}player',f'team_CTF_{team}spawn']
        for classname in classes:
            entities.append(entity({'classname':classname,'origin':vector(spawn['origin']),'angle':spawn['angle']}))
    for pickup in level['pickups']:
        entities.append(entity({'classname':pickup['classname'],'origin':vector(pickup['origin'])}))
    for light in level['lighting']['lights']:
        entities.append(entity({'classname':'light','targetname':light['id'],'origin':vector(light['origin']),
                                '_color':vector(light['color']),'light':light['intensity']}))
    worldspawn = {'classname':'worldspawn','message':level['name'],'_minlight':level['lighting']['ambient']}
    if level['lighting'].get('directional',False):
        worldspawn['_aftershock_deluxe'] = 1
    return entity(worldspawn,world)+''.join(entities)
