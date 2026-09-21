"""JSON Schemas for authored data; semantic/resource checks remain in the loaders."""
import json
import math
from pathlib import Path

from jsonschema import Draft202012Validator
from jsonschema.exceptions import best_match

ROOT = Path(__file__).resolve().parents[2]
KINDS = ('level', 'weapon', 'animation', 'material', 'effect', 'match-spec')


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
                  lighting=lighting, viewpoints=array(view,0,64))
    return obj(fields,[key for key in fields if key!='viewpoints'])


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
                       size=num(.001,4096),velocity=vector(-65536,65536,False),gravity=vector(-65536,65536,False),
                       drag=num(0,100),collision=dict(type='boolean'),soft=dict(type='boolean'),lit=dict(type='boolean'),
                       color=vector(0,1,False,4),flipbook=obj(dict(columns=num(1,64,True),rows=num(1,64,True),fps=num(.01,1000)))),
                  ['name','kind','material','capacity','rate','burst','lifetime_ms','size'])
    emitter['allOf'] = [{'if':dict(properties=dict(kind=dict(const='mesh'))),'then':dict(required=['model'])}]
    return obj(dict(version=dict(const=1),name=qpath(31),emitters=array(emitter,1,32)))


def match_schema():
    identifier = dict(type='string',pattern='^[a-zA-Z0-9_-]{1,64}$')
    secret = dict(type='string',pattern='^[a-zA-Z0-9_.-]{8,128}$')
    schema = obj(dict(id=identifier,map=dict(identifier,maxLength=48),mode=num(0,4,True),frag_limit=num(0,10000,True),
                      time_limit=num(0,1440,True),players=num(1,64,True),password=secret,token=secret),
                 ['id','map','players','password','token'])
    schema['anyOf'] = [dict(required=[key],properties={key:dict(minimum=1)}) for key in ('frag_limit','time_limit')]
    return schema


def schema(kind):
    schemas = dict(level=level_schema,weapon=weapon_schema,animation=animation_schema,material=material_schema,
                   effect=effect_schema,**{'match-spec':match_schema})
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
    elif kind == 'weapon':
        example = json.loads((ROOT/'tests/assets/weapons/rifle.weapon.json').read_text())
        example.update(recoil=[[0,0]],materials=[example['materials'][0]],attachments=[],sounds={})
    elif kind == 'animation':
        example = dict(version=1,model_source='rifle.gltf',model_asset='models/anim_rifle.iqm',initial_state='idle',
                       states=[dict(name='idle',clip='idle',loop=True)])
    elif kind == 'material':
        example = dict(alphaMode='OPAQUE')
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
