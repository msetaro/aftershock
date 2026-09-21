"""The v2 level contract; building on v1 keeps shared entity/material fields aligned."""
import copy

from tools.agent.formats import obj, num, array, vector, enum


def version2(legacy):
    fields = copy.deepcopy(legacy['properties'])
    for key in ('rooms','connections','cover'):
        del fields[key]
    fields['props']['items']['properties'].update(angle=num(-360,360),bounds_center=vector(integer=False))
    identity = fields['name']
    xy = vector(integer=False,count=2)
    ring = array(xy,3,256)
    holes = array(ring,0,16)
    rectangle = obj(dict(center=xy,size=vector(.001,8192,False,2),angle=num(-360,360)),['center','size'])
    circle = obj(dict(center=xy,radius=num(.001,8192),tolerance=dict(num(.001,512),description='Curve tolerance must be positive and less than its radius')))
    arc = obj(dict(circle['properties'],start=num(-360,360),end=num(-360,720),thickness=num(1,1024)))
    shape = {'oneOf':[obj(dict(polygon=ring,holes=holes),['polygon']),obj(dict(rectangle=rectangle)),
                      obj(dict(circle=circle)),obj(dict(arc=arc)),
                      obj(dict(path=obj(dict(points=array(xy,2,256),thickness=num(1,1024)))))]}
    opening = obj(dict(edge=num(0,255,True),at=num(0,64000),width=num(32,8192),sill=num(0,8192),height=num(1,8192)))
    rule = obj(dict(face_point=xy,spacing=num(64,8192),width=num(32,8192),sill=num(0,8192),height=num(1,8192),floors=array(num(0,7,True),1,8)),['face_point','spacing','width','sill','height'])
    item = obj(dict(id=identity,kind=enum('solid','building','platform','wall','overhead','transition'),shape=shape,base=num(-32000,32000),height=num(1,8192),
                    material=enum(*fields['materials']['properties']),wall_thickness=num(4,128),floors=num(1,8,True),
                    openings=array(opening),opening_rules=array(rule,0,16),roof_access=dict(type='boolean'),transition=enum('stairs','ramp'),descending=dict(type='boolean'),bullet_solid=dict(type='boolean')),['id','kind','shape','base','height'])
    item['allOf'] = [{'if':dict(properties=dict(kind=dict(const='transition'))),'then':dict(required=['transition'])}]
    fields.update(version=dict(const=2),boundary=obj(dict(polygon=ring,holes=holes,floor=num(-32000,32000),ceiling=num(-32000,32000)),
                  ['polygon','floor','ceiling']),shapes=array(item,1))
    return obj(fields,[key for key in fields if key!='viewpoints'])
