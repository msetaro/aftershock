"""JSON Schemas for authored data; semantic/resource checks remain in the loaders."""
import json
import math
from pathlib import Path

from jsonschema import Draft202012Validator
from jsonschema.exceptions import best_match

ROOT = Path(__file__).resolve().parents[2]
KINDS = ('level', 'weapon', 'animation', 'material', 'effect', 'decal', 'post', 'entities', 'ui', 'sound-event', 'navigation', 'behavior', 'match-spec')


def obj(properties, required=None, extra=False):
    return dict(type='object', properties=properties, required=list(properties) if required is None else required,
                additionalProperties=extra)


def num(low, high, integer=False):
    return dict(type='integer' if integer else 'number', minimum=low, maximum=high)


def array(items, low=0, high=128):
    return dict(type='array', items=items, minItems=low, maxItems=high)


def vector(low=-32000, high=32000, integer=True, count=3):
    return array(num(low, high, integer), count, count)


def enum(*values):
    return dict(enum=list(values))


def qpath(length=63):
    return dict(type='string', pattern=r'^(?!/)(?!.*\.\.)[a-z0-9_./-]+$', maxLength=length,description='Use a relative asset path without parent traversal')


def level_schema():
    identity = dict(type='string', pattern='^[a-z][a-z0-9_]{0,31}$')
    materials = ('floor', 'wall', 'trim', 'cover', 'prop', 'sky')
    room = obj(dict(id=identity, origin=vector(), size=vector(64,8192)))
    room['properties']['size']['prefixItems'] = [dict(num(64,8192,True),multipleOf=2)]*2
    connection = obj({'id': identity, 'from': identity, 'to': identity, 'axis': enum('x','y'),
                      'at': num(-32000,32000,True), 'width': dict(num(64,8192,True), multipleOf=2,description='Corridor width must meet player/design clearance'),
                      'height': dict(num(80,8192,True),description='Door height must meet player/design clearance'), 'door': dict(type='boolean'), 'transition': enum('stairs','ramp',None)},
                     ['id','from','to','axis','at','width','height'])
    cover = obj(dict(id=identity, origin=vector(), kit=enum('low','tall'), size=vector(16,2048)), ['id','origin','kit'])
    prop = obj(dict(id=identity, model=qpath(59), origin=vector(), size=vector(1,2048),
                    material=enum(*materials), solid=dict(type='boolean')))
    spawn = obj(dict(team=enum('ffa','red','blue'), origin=vector(), angle=num(0,359,True)))
    pickups = ('weapon_gauntlet weapon_machinegun weapon_shotgun weapon_grenadelauncher weapon_rocketlauncher '
               'weapon_lightning weapon_railgun weapon_plasmagun weapon_bfg ammo_bullets ammo_shells ammo_grenades '
               'ammo_rockets ammo_lightning ammo_slugs ammo_cells ammo_bfg item_health item_health_small '
               'item_health_large item_health_mega item_armor_shard item_armor_combat item_armor_body '
               'team_CTF_redflag team_CTF_blueflag').split()
    light = obj(dict(id=identity, origin=vector(), color=vector(0,1,False), intensity=num(1,100000,True)))
    sun = obj(dict(direction=vector(integer=False), color=vector(0,1,False), intensity=num(1,100000,True)))
    lighting = obj(dict(ambient=num(0,255,True), lights=array(light), sun=sun, directional=dict(type='boolean')),
                   ['ambient','lights'])
    view = obj(dict(id=dict(identity, pattern='^(?!auto_)[a-z][a-z0-9_]{0,31}$',description='Reserved viewpoint identifiers begin with auto_; choose another id'),
                    origin=vector(), angles=vector(-360,360,False)))
    fields = dict(version=dict(const=1), name=identity, materials=obj({key:dict(qpath(59),pattern=r'^(?!/)(?!.*(?:^|/)\.{1,2}(?:/|$))[a-z0-9_./-]+$') for key in materials}),
                  rules=obj({key:num(1,64000,True) for key in ('min_corridor_width','min_door_height','max_sightline','max_cover_gap')}),
                  rooms=array(room,1), connections=array(connection), spawns=array(spawn,1), cover=array(cover,1),
                  props=array(prop), pickups=array(obj(dict(classname=enum(*pickups),origin=vector()))),
                  lighting=lighting, viewpoints=array(view,0,64),
                  audio_zones=array(obj(dict(mins=vector(integer=False),maxs=vector(integer=False),
                                             wet=num(0,1),decay=num(.1,10),damping=num(0,.95))),0,32))
    legacy = obj(fields,[key for key in fields if key not in ('viewpoints','audio_zones')])
    from tools.level.schema import version2
    return {'if':dict(properties=dict(version=dict(const=2)),required=['version']),
            'then':version2(legacy),'else':legacy}


def weapon_schema():
    fields = dict(version=dict(const=2), name=qpath(), model=qpath(), animation=qpath(),
                  fire_mode=enum('auto','semi','burst'), interval_ms=num(20,60000,True), burst_count=num(1,32,True),
                  ballistics=enum('hitscan','projectile'))
    for key, maximum in [('damage',10000),('minimum_damage',10000),('falloff_start',65536),('falloff_end',65536),
                         ('range',65536),('spread_degrees',90),('ads_spread_scale',4),('view_kick_scale',4),('sway',10),('bob',10)]:
        fields[key] = num(0,maximum)
    fields.update(ads_ms=num(20,10000,True), ads_fov=num(1,179), magazine=num(1,1000,True),
                  reserve=num(0,65535,True), switch_ms=num(20,10000,True), recoil=array(vector(-90,90,False,2),1,32))
    reload = obj(dict(time_ms=num(0,10000,True), event=qpath(), action=enum('eject','insert','chamber','finish'),
                      cancel=dict(type='boolean')),extra=True)
    fields['reload'] = dict(array(reload,4,4), prefixItems=[dict(reload,properties=dict(reload['properties'],action=dict(const=action)))
                           for action in ('eject','insert','chamber','finish')])
    fields['projectile'] = obj(dict(model=qpath(), speed=num(1,8192), gravity=num(0,4096), bounce=num(0,1),
                                   radius=num(0,4096), fuse_ms=num(20,60000,True), size=num(.125,32)),extra=True)
    fields['melee'] = obj(dict(range=num(0,256), damage=num(0,10000), interval_ms=num(20,60000,True)),extra=True)
    surface = obj(dict(name=qpath(31), surface_flags=num(0,4294967295,True), depth=num(0,256),
                       damage_scale=num(0,1), effect=qpath()),extra=True)
    fields['materials'] = dict(array(surface,1,8), prefixItems=[dict(surface,properties=dict(surface['properties'],surface_flags=dict(const=0)))])
    fields['attachments'] = array(obj(dict(name=qpath(31),socket=qpath(31),model=qpath(),spread_scale=num(0,4),
                                           recoil_scale=num(0,4),ads_fov=num(1,179)),extra=True),0,8)
    fields['sounds'] = dict(type='object',propertyNames=qpath(31),additionalProperties=qpath(),maxProperties=8)
    return obj(fields,extra=True)


def animation_schema():
    name = dict(type='string',minLength=1,maxLength=63,pattern='^[^\u0000]+$')
    scalar = num(-3.4028234e38,3.4028234e38)
    parameter = obj(dict(name=name,default=scalar,min=scalar,max=scalar),['name'],True)
    mask = obj(dict(name=name,root=name,weights=dict(type='object',additionalProperties=num(0,1))),['name'],True)
    node = obj(dict(name=name,clip=name,blend=array(name,2,2),additive=array(name,2,2),reference=name,
                    weight=num(0,1),parameter=name,mask=name,loop=dict(type='boolean')),['name'],True)
    node['oneOf'] = [dict(required=[key]) for key in ('clip','blend','additive')]
    node['allOf'] = [{'if':dict(required=['additive']),'then':dict(required=['reference'])}]
    event = obj(dict(name=name,time_ms=num(0,4294967295,True),bone=name),['name','time_ms'],True)
    state = obj(dict(name=name,clip=name,node=name,loop=dict(type='boolean'),speed=num(1/65536,16),events=array(event,0,65536)),['name','clip'],True)
    condition = obj(dict(parameter=name,op=enum('==','!=','<','<=','>','>='),value=scalar),extra=True)
    transition = obj({'from':name,'to':name,'blend_ms':num(0,60000,True),'on_end':dict(type='boolean'),
                      'conditions':array(condition,0,65536)},['from','to'],True)
    box = obj(dict(name=name,bone=name,offset=vector(-3.4028234e38,3.4028234e38,False),
                   extent=array(dict(scalar,minimum=0,exclusiveMinimum=0),3,3)),['name','bone','extent'],True)
    return obj(dict(version=dict(const=1),model_source=dict(type='string',minLength=1),
                    model_asset=dict(qpath(),pattern=r'^(?!/)(?!.*\.\.)[a-z0-9_/-]+\.iqm$'),
                    initial_state=name,parameters=array(parameter,0,16),masks=array(mask,0,16),nodes=array(node,0,64),
                    states=array(state,1,64),transitions=array(transition,0,65536),hit_boxes=array(box,0,32)),
               ['version','model_source','model_asset','initial_state','states'],True)


def material_schema():
    # The recipe chooses legacy versus metallic-roughness. Describe both source
    # shapes; validation with a recipe checks the selected branch as well.
    color = vector(0,1,False,4)
    channel = obj(dict(uri=dict(type='string',minLength=1),texCoord=dict(const=0),extensions=obj({})),['uri'],True)
    pbr = obj(dict(baseColorFactor=color, metallicFactor=num(0,1),roughnessFactor=num(0,1),
                   baseColorTexture=channel,metallicRoughnessTexture=channel,extensions=obj({})),[],True)
    normal = dict(channel,properties=dict(channel['properties'],scale=num(-3.4028234e38,3.4028234e38)))
    return obj(dict(baseColorFactor=color,texture=dict(type='string',minLength=1),unlit=dict(type='boolean'),
                    pbrMetallicRoughness=pbr,emissiveFactor=vector(0,1,False),normalTexture=normal,emissiveTexture=channel,
                    alphaMode=enum('OPAQUE','MASK','BLEND'),alphaCutoff=num(0,1),doubleSided=dict(type='boolean'),
                    extensions=obj(dict(KHR_materials_unlit=obj({})),[]),occlusionTexture={'not': {}}),[],True)


def effect_schema():
    emitter = obj(dict(name=qpath(31),kind=enum('sprite','mesh','trail'),material=qpath(),model=qpath(),
                       capacity=num(1,4096,True),rate=num(0,10000),burst=num(0,4096,True),lifetime_ms=num(1,60000,True),
                       size=num(.001,4096),end_size=num(.001,4096),velocity_spread=vector(0,65536,False),origin_spread=vector(0,4096,False),
                       rotation=num(-360,360),rotation_spread=num(0,360),angular_velocity=num(-3600,3600),
                       light=obj(dict(radius=num(0,4096),intensity=num(0,128),color=vector(0,1,False))),velocity=vector(-65536,65536,False),gravity=vector(-65536,65536,False),
                       drag=num(0,100),collision=dict(type='boolean'),soft=dict(type='boolean'),lit=dict(type='boolean'),
                       color=vector(0,1,False,4),flipbook=obj(dict(columns=num(1,64,True),rows=num(1,64,True),fps=num(.01,1000)))),
                  ['name','kind','material','capacity','rate','burst','lifetime_ms','size'])
    emitter['allOf'] = [{'if':dict(properties=dict(kind=dict(const='mesh'))),'then':dict(required=['model'])}]
    return obj(dict(version=dict(const=1),name=qpath(31),decal=qpath(),emitters=array(emitter,1,32)),['version','name','emitters'])


def sound_event_schema():
    layer = obj(dict(role=enum('mechanical','tail','distant'),sample=qpath(),gain=num(0,2),
                     min_distance=num(0,65536),max_distance=num(1,65536)))
    return obj(dict(version=dict(const=1),name=qpath(31),bus=enum('weapons','ambient','music','voice','ui'),
                    group=num(0,15,True),priority=num(0,255,True),voice_limit=num(1,96,True),
                    distance_model=enum('linear','inverse'),reference_distance=num(.01,65535),
                    max_distance=num(1,65536),rolloff=num(0,16),doppler=dict(type='boolean'),
                    occlusion=dict(type='boolean'),reverb_send=num(0,1),layers=array(layer,1,4)))


def post_schema():
    return obj(dict(version=dict(const=1),name=qpath(31),lut=qpath(),exposure_ev=num(-12,12),
                    sharpen=num(0,1),vignette=num(0,1),grain=num(0,1),lut_strength=num(0,1),
                    focus_distance=num(1,65536),focus_range=num(1,65536),dof_radius=num(0,8),motion_blur=num(0,1)),
               ['version','name'])


def decal_schema():
    return obj(dict(version=dict(const=1),name=qpath(31),color_map=qpath(),normal_map=qpath(),
                    size=vector(.01,4096,False),lifetime_ms=num(1,600000,True),fade_ms=num(1,600000,True),
                    color=vector(0,1,False,4),normal_strength=num(0,4)))


def match_schema():
    identifier = dict(type='string',pattern='^[a-zA-Z0-9_-]{1,64}$')
    secret = dict(type='string',pattern='^[a-zA-Z0-9_.-]{8,128}$')
    schema = obj(dict(id=identifier,map=dict(identifier,maxLength=48),mode=num(0,4,True),frag_limit=num(0,10000,True),
                      time_limit=num(0,1440,True),players=num(1,64,True),password=secret,token=secret),
                 ['id','map','players','password','token'])
    schema['anyOf'] = [dict(required=[key],properties={key:dict(minimum=1)}) for key in ('frag_limit','time_limit')]
    return schema


def entities_schema():
    identity=dict(type='string',pattern='^[a-z][a-z0-9_]{0,62}$')
    def field(schema, key):
        return dict(schema, **{'x-spawn-field':key})
    components=obj(dict(
        transform=obj(dict(origin=field(vector(integer=False),'origin'), angles=field(vector(-360,360,False),'angles')),[]),
        pickup=obj(dict(amount=field(num(1,10000,True),'count')),[]),
        hooks=obj(dict(target=field(identity,'target'),targetname=field(identity,'targetname')),[]),
        replication=obj(dict(priority=field(num(0,3,True),'rep_priority'),radius=field(num(0,32768),'rep_radius')),[]),
        model=obj(dict(resource=field(qpath(),'model')),[]),
        animation=obj(dict(first_frame=field(num(0,4095,True),'anim_first'),frames=field(num(1,4096,True),'anim_frames'),
                           frame_ms=field(num(10,10000,True),'anim_ms'),loop=field(dict(type='boolean'),'anim_loop')),[]),
        collision=obj(dict(mins=field(vector(-1024,1024,False),'mins'),maxs=field(vector(-1024,1024,False),'maxs'),
                           solid=field(dict(type='boolean'),'solid')),[]),
        trigger=obj(dict(wait_ms=field(num(0,60000,True),'trigger_wait'),once=field(dict(type='boolean'),'trigger_once')),[]),
        damage=obj(dict(amount=field(num(0,10000,True),'dmg'),health=field(num(0,10000,True),'health'),
                        splash=field(num(0,10000,True),'splash_damage'),radius=field(num(0,4096),'splash_radius')),[]),
        audio=obj(dict(sound=field(qpath(),'noise'),loop=field(dict(type='boolean'),'audio_loop')),[])),[])
    definition=obj(dict(id=identity,extends=identity,
                        native=dict(type='string',pattern='^[a-zA-Z][a-zA-Z0-9_]{0,62}$'),
                        components=components),['id','components'])
    return obj(dict(version=dict(const=1),name=dict(identity,maxLength=31),definitions=array(definition,1,256)))


def ui_schema():
    identity=dict(type='string',pattern='^[a-z][a-z0-9_]{0,30}$')
    localized=dict(type='object',minProperties=1,maxProperties=8,
                   propertyNames=dict(type='string',pattern='^[a-z]{2,3}(-[a-z0-9]{2,8})?$'),
                   additionalProperties=dict(type='string',minLength=1,maxLength=256,pattern=r'^[^\x00-\x1f\x7f]+$'))
    item=obj(dict(id=identity,kind=enum('label','button','slider','binding','value'),text=identity,
                  rect=vector(-8192,8192,False,4),anchor=vector(0,1,False,2),color=vector(0,1,False,4),
                  action=enum('none','page','resume','map','quit'),
                  target=dict(type='string',maxLength=63,pattern=r'^[a-z0-9_+./-]*$'),
                  range=vector(0,30,False,3)),['id','kind','text','rect','anchor'])
    return obj(dict(version=dict(const=1),name=identity,font=qpath(),canvas=vector(320,8192,True,2),
                    locales=array(obj(dict(id=dict(type='string',pattern='^[a-z]{2,3}(-[a-z0-9]{2,8})?$'),direction=enum('ltr','rtl'))),1,8),
                    texts=array(obj(dict(id=identity,size=num(12,96,True),values=localized)),1,64),
                    pages=array(obj(dict(id=identity,items=array(item,1,128))),3,16)))


def behavior_schema():
    identity = dict(type='string', pattern='^[a-z][a-z0-9_]{0,30}$')
    rule = obj(dict(to=identity, field=enum('visible','heard','health','covered','time_ms','distance'),
                    op=enum('eq','ne','lt','le','gt','ge'), value=num(0,3600000), min_ms=num(0,600000,True)))
    state = obj(dict(name=identity, parent=identity, action=enum('idle','patrol','investigate','cover','attack'),
                     transitions=array(rule,0,16)), ['name','action','transitions'])
    return obj(dict(version=dict(const=1), name=identity, initial=identity, states=array(state,1,32)))


def navigation_schema():
    agent = obj(dict(radius=num(.125,128), height=num(8,256), climb=num(0,255), slope=num(.1,84.9)))
    link = obj(dict(id=num(1,4294967295,True), start=vector(-131072,131072,False),
                    end=vector(-131072,131072,False), radius=num(.125,256),
                    bidirectional=dict(type='boolean'), kind=enum('jump','drop','door')))
    return obj(dict(version=dict(const=1), collision=qpath(), agent=agent,
                    cell_size=num(1,32), cell_height=num(.5,16), links=array(link,0,256)))


def schema(kind):
    schemas = dict(behavior=behavior_schema,navigation=navigation_schema,level=level_schema,weapon=weapon_schema,animation=animation_schema,material=material_schema,
                   effect=effect_schema,decal=decal_schema,post=post_schema,entities=entities_schema,ui=ui_schema,**{'match-spec':match_schema,'sound-event':sound_event_schema})
    schema = schemas[kind]()
    schema['$schema'] = 'https://json-schema.org/draft/2020-12/schema'
    return schema


def describe(kind):
    if kind == 'level':
        example = dict(version=1,name='one_room',materials={role:'level/'+role for role in ('floor','wall','trim','cover','prop','sky')},
                       rules=dict(min_corridor_width=64,min_door_height=80,max_sightline=1024,max_cover_gap=512),
                       rooms=[dict(id='room',origin=[0,0,0],size=[512,512,192])],connections=[],
                       spawns=[dict(team='ffa',origin=[-128,0,24],angle=0)],cover=[dict(id='cover',origin=[0,0,0],kit='low')],
                       props=[],pickups=[],lighting=dict(ambient=32,lights=[]))
    elif kind == 'behavior':
        example = dict(version=1,name='guard',initial='patrol',states=[dict(name='patrol',action='patrol',transitions=[])])
    elif kind == 'navigation':
        example = dict(version=1,collision='maps/two_lane.bsp',agent=dict(radius=15,height=56,climb=18,slope=46),
                       cell_size=4,cell_height=2,links=[])
    elif kind == 'entities':
        example = json.loads((ROOT/'tests/assets/entities/pickups.json').read_text())
    elif kind == 'ui':
        example = json.loads((ROOT/'tests/assets/ui/shell.json').read_text())
    elif kind == 'weapon':
        example = json.loads((ROOT/'tests/assets/weapons/rifle.weapon.json').read_text())
        example.update(recoil=[[0,0]],materials=[example['materials'][0]],attachments=[],sounds={})
    elif kind == 'animation':
        example = dict(version=1,model_source='rifle.gltf',model_asset='models/anim_rifle.iqm',initial_state='idle',
                       states=[dict(name='idle',clip='idle',loop=True)])
    elif kind == 'material':
        example = dict(alphaMode='OPAQUE')
    elif kind == 'sound-event':
        example = dict(version=1,name='rifle',bus='weapons',group=0,priority=100,voice_limit=8,
                       distance_model='inverse',reference_distance=80,max_distance=4096,rolloff=1,
                       doppler=True,occlusion=True,reverb_send=.25,layers=[
                           dict(role='mechanical',sample='sound/rifle.wav',gain=1,min_distance=0,max_distance=4096)])
    elif kind == 'post':
        example = dict(version=1,name='filmic',exposure_ev=0)
    elif kind == 'decal':
        example = dict(version=1,name='bullet',color_map='textures/bullet.ktx2',normal_map='textures/bullet_n.ktx2',
                       size=[16,16,4],lifetime_ms=10000,fade_ms=2000,color=[1,1,1,1],normal_strength=1)
    elif kind == 'effect':
        example = dict(version=1,name='spark',emitters=[dict(name='spark',kind='sprite',material='materials/spark.asmat',
                       capacity=32,rate=0,burst=8,lifetime_ms=250,size=2)])
    else:
        example = dict(id='local-1',map='two_lane',mode=0,frag_limit=10,time_limit=10,players=2,
                       password='local-secret',token='allocation-secret')
    notes = 'Schema checks structure/ranges; cook/build still checks references, resources and semantic constraints.'
    if kind == 'effect':
        notes = 'Cook kind effect into .asfx; the native effects controls load and play its bounded presentation emitters.'
    return dict(kind=kind,schema=schema(kind),example=example,notes=notes)


def validate(kind, document, source):
    error = best_match(Draft202012Validator(schema(kind)).iter_errors(document))
    if error:
        path = '$'+''.join(f'[{part}]' if isinstance(part,int) else '.'+part for part in error.absolute_path)
        message = error.message
        if error.validator in ('required', 'additionalProperties'):
            message = 'missing or unknown fields: '+message
        elif error.validator == 'type':
            message = 'expected '+str(error.validator_value)+'; '+message
        raise ValueError(dict(file=str(source),path=path,hint=message+'; '+error.schema.get('description', f'use tools/agent describe {kind} for a valid example')))
    pending = [('$',document)]
    while pending:
        path, value = pending.pop()
        if isinstance(value,float) and not math.isfinite(value):
            raise ValueError(dict(file=str(source),path=path,hint='expected a finite JSON number; replace NaN/infinity'))
        if isinstance(value,dict):
            pending.extend((path+'['+json.dumps(key)+']',item) for key,item in value.items())
        elif isinstance(value,list):
            pending.extend((path+f'[{index}]',item) for index,item in enumerate(value))
    return document


def diagnostic(error, source, hint):
    if error.args and isinstance(error.args[0], dict) and 'path' in error.args[0]:
        return error.args[0]
    return dict(file=str(source), path='$', hint=str(error)+'; '+hint)
