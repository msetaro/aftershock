"""Shooter design evidence from the local agent's compiled-world collision queries."""
import math

from shapely import Point

from polygons import pieces,navigation


def analyze(engine,level,limits=None):
    limits=limits or {}
    errors=[]
    queries=0
    def error(code,identity,message,suggestion):
        errors.append(dict(code=code,id=identity,message=message,suggestion=suggestion))
    def trace(start,end,hull='point'):
        nonlocal queries
        queries+=1
        if queries>8192:
            raise ValueError('shooter report exceeds 8192 trace queries; reduce spawn/intent complexity')
        return engine.request('trace',start=start,end=end,hull=hull)
    def eye(point,height=50):
        return [point[0],point[1],(point[2] if len(point)>2 else level['boundary']['floor'])+height]
    def visible(a,b):
        result=trace(a,b)
        return result['fraction']==1 and not result['start_solid'],result
    spawns=level['spawns']
    spawn_eyes=[[x,y,z+26] for x,y,z in (s['origin'] for s in spawns)]
    for i,spawn in enumerate(spawns):
        x,y,z=spawn['origin']
        result=trace([x,y,z+.25],[x,y,z+.25],'player')
        if result['start_solid'] or result['all_solid']:
            error('spawn_clearance',str(i),'Spawn intersects compiled player collision.','Move the spawn into empty space with full standing clearance.')
        for j in range(i):
            clear,_=visible(spawn_eyes[i],spawn_eyes[j])
            if clear:
                error('spawn_visibility',f'{j}/{i}','Spawns have a direct compiled-world sightline.','Move a spawn behind solid cover or add a sightline break.')
    area,records=pieces(level)
    routes=[]
    navigation(level,records,routes)
    first_contact=[]
    for index,route in enumerate(routes[1:],1):
        if spawns[0]['team']==spawns[index]['team'] and spawns[0]['team']!='ffa':
            continue
        distance=0
        for i in range((len(route)+1)//2):
            if i:
                distance+=math.dist(route[i-1],route[i])
            clear,_=visible(eye(route[i]),eye(route[-i-1]))
            if clear:
                first_contact.append(dict(spawns=[0,index],estimated_seconds=round(distance/320,3),
                    method='symmetric travel along the clearance path at 320 units/s; compiled-world sight query'))
                break
    intent_reports=[]
    objectives=[]
    for intent in level.get('intents',[]):
        identity,kind,points=intent['id'],intent['kind'],intent['points']
        row=dict(id=identity,kind=kind)
        if not all(area.covers(Point(*p[:2])) for p in points):
            error('intent_bounds',identity,'An annotated point is outside the playable boundary.','Move the annotation inside the boundary and outside its holes.')
            intent_reports.append(row)
            continue
        if kind=='sightline':
            if len(points)!=2:
                raise ValueError('sightline intent requires two points: '+identity)
            clear,hit=visible(eye(points[0]),eye(points[1]))
            row.update(clear=clear,expected_blocked=intent.get('blocked',False),trace=hit)
            if hit['start_solid'] or clear==intent.get('blocked',False):
                error('intent_sightline',identity,'Compiled sightline disagrees with the drawing.','Move the endpoints or change solid cover; keep the annotated expectation explicit.')
        elif kind=='route':
            if len(points)<2:
                raise ValueError('route intent requires at least two points: '+identity)
            distance=sum(math.dist(eye(a,0),eye(b,0)) for a,b in zip(points,points[1:]))
            blocked=[]
            for index,(a,b) in enumerate(zip(points,points[1:])):
                hit=trace(eye(a,24.25),eye(b,24.25),'player')
                if hit['start_solid'] or hit['fraction']<1:
                    blocked.append(index)
            row.update(distance=round(distance,3),geometric_seconds=round(distance/320,3),blocked_segments=blocked,playtest_required=True)
            if blocked and len({eye(p,0)[2] for p in points})==1:
                error('intent_route',identity,'The annotated ground route crosses player collision.','Move its waypoints around solid geometry; use the movement playtest for stairs and timing.')
        elif kind=='objective':
            if len(points)!=1:
                raise ValueError('objective intent requires one point: '+identity)
            objectives.append(intent)
            exposed=[i for i,p in enumerate(spawn_eyes) if visible(p,eye(points[0]))[0]]
            row['visible_from_spawns']=exposed
            if exposed:
                error('objective_visibility',identity,'Objective is directly visible from spawn(s) '+str(exposed)+'.','Move the objective behind cover or move the exposed spawns.')
            if intent.get('mode')=='ctf':
                classname={'red':'team_CTF_redflag','blue':'team_CTF_blueflag'}.get(intent.get('team'))
                if not classname or not any(p['classname']==classname and math.dist(p['origin'][:2],points[0][:2])<=16 for p in level['pickups']):
                    error('objective_entity',identity,'CTF objective has no matching authored team flag.','Place its native team flag at the annotated objective point.')
            else:
                error('objective_mode',identity,'This objective has no supported native mode.','Use a red/blue CTF objective; use hold or engagement for tactical annotations.')
        elif kind in ('hold','engagement'):
            center=[sum(p[k] for p in points)/len(points) for k in (0,1)]
            center.append(sum(eye(p,0)[2] for p in points)/len(points))
            radius=intent.get('radius',128)
            position=eye(center,24.25)
            occupied=trace(position,position,'player')
            if occupied['start_solid'] or occupied['all_solid']:
                error('intent_position',identity,'The tactical position intersects compiled player collision.','Move the hold/engagement center into standing space.')
            row.update(center=center,radius=radius,visible_directions=sum(visible(eye(center),eye([center[0]+radius*math.cos(i*math.pi/4),center[1]+radius*math.sin(i*math.pi/4),center[2]]))[0] for i in range(8)))
            if row['visible_directions']<intent.get('min_visible_directions',1):
                error('intent_view',identity,'The tactical position has fewer clear directions than required.','Move its center or add an opening toward the intended engagement area.')
        else:
            raise ValueError('unknown intent kind: '+kind)
        intent_reports.append(row)
    if objectives and any(i.get('mode')=='ctf' for i in objectives):
        if sorted(i.get('team','') for i in objectives if i.get('mode')=='ctf')!=['blue','red']:
            error('objective_pair','ctf','CTF needs exactly one red and one blue objective.','Author both flags and ensure both teams can reach them.')
    x,y,X,Y=area.bounds
    reach=math.hypot(X-x,Y-y)
    bins=[128,256,512,1024,2048,4096,8192]
    histogram=[0]*(len(bins)+1)
    distances=[]
    lanes=[]
    near_cover=0
    stride=max(4,math.ceil(sum(len(r) for r in routes)/128))
    for route_index,route in enumerate(routes):
        for index in sorted(set(range(0,len(route),stride))|{len(route)-1}):
            point=route[index]
            origin=eye(point)
            cover=[]
            for direction in range(8):
                angle=direction*math.pi/4
                end=[max(-32752,min(32752,point[0]+reach*math.cos(angle))),max(-32752,min(32752,point[1]+reach*math.sin(angle))),origin[2]]
                hit=trace(origin,end)
                distance=math.dist(origin,hit['end'])
                distances.append(distance)
                histogram[next((i for i,limit in enumerate(bins) if distance<=limit),len(bins))]+=1
                low_start,low_end=origin[:],end[:]
                low_start[2]-=18
                low_end[2]-=18
                cover.append(math.dist(low_start,trace(low_start,low_end)['end']))
            near_cover+=min(cover)<=limits.get('cover_radius',128)
            other=route[min(index+1,len(route)-1)] if index<len(route)-1 else route[max(0,index-1)]
            yaw=math.atan2(other[1]-point[1],other[0]-point[0])+math.pi/2
            width=30
            center=eye(point,24.25)
            for sign in (-1,1):
                end=[point[0]+sign*256*math.cos(yaw),point[1]+sign*256*math.sin(yaw),center[2]]
                width+=math.dist(center,trace(center,end,'player')['end'])
            lanes.append(dict(route=route_index,origin=list(point),width=round(width,3),choke=width<96))
    cover_density=near_cover/len(lanes) if lanes else 0
    maximum=max(distances,default=0)
    if maximum>limits.get('max_sightline',level['rules']['max_sightline'])+.25:
        error('long_sightline','lanes','A sampled compiled sightline exceeds the design limit.','Add a bend or solid cover to break the longest lane.')
    if cover_density<limits.get('min_cover_density',0):
        error('sparse_cover','lanes','Too few sampled lane positions have nearby waist-height cover.','Add low cover along exposed lanes while preserving walking clearance.')
    return dict(passed=not errors,stage='static compiled-world analysis; annotated routes require movement playback',
                errors=errors,intents=intent_reports,trace_queries=queries,lanes=lanes,cover_density=round(cover_density,4),
                first_contact=first_contact,sightlines=dict(samples=len(distances),histogram=histogram,upper_bounds=bins,
                maximum=round(maximum,3),method='eight horizontal rays per sampled clearance-path position'))


def play_routes(engine,level,output,seed=164):
    """Reuse a session already configured for 20 ms; time actual player input/physics."""
    import json
    from tools.agent.playtest import run_script
    output.mkdir(parents=True,exist_ok=True)
    results=[]
    for intent in level.get('intents',[]):
        if intent['kind']!='route':
            continue
        points=[[p[0],p[1],(p[2] if len(p)>2 else level['boundary']['floor'])+24] for p in intent['points']]
        if len(points)<2:
            raise ValueError('route needs at least two points: '+intent['id'])
        folder=output/intent['id']
        folder.mkdir()
        start=' '.join(f'{v:.9g}' for v in points[0])
        steps=[dict(op='request',request=dict(op='exec',command='setviewpos '+start+' 0')),
               dict(op='step',frames=5),dict(op='request',request=dict(op='state'))]
        for a,b in zip(points,points[1:]):
            frames=min(1500,max(100,math.ceil(math.dist(a,b)/320/.02)*2+100))
            steps.append(dict(op='walk',position=b,tolerance=8,max_frames=frames))
        script=dict(version=1,dt=20,seed=seed,warmup=150,steps=steps)
        (folder/'playtest.json').write_text(json.dumps(script,sort_keys=True,indent=2)+'\n')
        report=run_script(engine,script,level['name'],folder,configure_session=False)
        result=dict(id=intent['id'],passed=report['ok'],report=intent['id']+'/report.json',measured_seconds=None)
        if report['ok']:
            begin=report['results'][2]['reply']['time']
            finish=next(r['state']['time'] for r in reversed(report['results']) if r['op']=='walk')
            result['measured_seconds']=(finish-begin)/1000
            if 'target_seconds' in intent:
                target=intent['target_seconds']
                tolerance=intent.get('tolerance_seconds',max(.5,target*.2))
                result.update(target_seconds=target,tolerance_seconds=tolerance)
                if abs(result['measured_seconds']-target)>tolerance:
                    result.update(passed=False,suggestion='Change route length/waypoints or explicitly revise the annotated timing target.')
        else:
            result.update(error=report['error'],suggestion='Move the blocked waypoint into connected standing space; add intermediate points for turns or elevation changes.')
        results.append(result)
    result=dict(passed=all(r['passed'] for r in results),routes=results,
                method='existing agent walk controller through native player input and physics; 20 ms simulation steps')
    (output/'routes.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    return result
