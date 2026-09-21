"""Run inside pinned Blender: original CC0 modular environment art, no characters."""
import argparse
import json
import math
from pathlib import Path
import random
import sys

import bpy

parser=argparse.ArgumentParser()
parser.add_argument('--parameters',type=Path,required=True)
parser.add_argument('--out',type=Path,required=True)
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
params=json.loads(args.parameters.read_text())
assert bpy.app.version==(5,0,1), 'Blender 5.0.1 required'
assert params['version']==1 and type(params['seed']) is int
width,height,thickness=(params[k] for k in ('bay_width','storey_height','wall_thickness'))
assert all(isinstance(v,(int,float)) and math.isfinite(v) and .1<=v<=32 for v in (width,height,thickness))
assert width>=2 and height>=2 and thickness<min(width,height)/4
rng=random.Random(params['seed'])
report=[]


def box(name,center,size,material):
    bpy.ops.mesh.primitive_cube_add(size=1,location=center)
    obj=bpy.context.object
    obj.name=name
    obj.scale=size
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    obj.data.materials.append(material)
    return obj


for name in ('facade','doorway','cornice','curb','stairs','fence','barrier','crate','sign'):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene=bpy.context.scene
    scene.render.engine='CYCLES'
    scene.cycles.device='CPU'
    scene.cycles.samples=1
    scene.cycles.seed=params['seed']
    scene.render.threads_mode='FIXED'
    scene.render.threads=1
    palette={}
    for label,color in [('stone',(.45,.43,.40,1)),('metal',(.12,.14,.16,1)),('wood',(.28,.16,.07,1)),('paint',(.07,.20,.09,1))]:
        material=bpy.data.materials.new(label)
        material.use_nodes=True
        node=material.node_tree.nodes.get('Principled BSDF')
        node.inputs['Base Color'].default_value=color
        node.inputs['Roughness'].default_value=.8
        palette[label]=material
    stone,metal,wood,paint=(palette[k] for k in ('stone','metal','wood','paint'))
    if name in ('facade','doorway'):
        opening_width=width*.5
        sill=height*.3 if name=='facade' else 0
        opening_top=height*.8
        side=(width-opening_width)/2
        for sign in (-1,1):
            box('jamb',(sign*(width/2-side/2),0,height/2),(side,thickness,height),stone)
        box('lintel',(0,0,(opening_top+height)/2),(opening_width,thickness,height-opening_top),stone)
        if sill:
            box('sill',(0,-thickness*.1,sill/2),(opening_width,thickness*1.25,sill),stone)
        box('cap',(0,0,height-.08),(width+.1,thickness+.12,.16),stone)
    elif name=='cornice':
        for i in range(3):
            box('moulding',(0,0,.08+i*.08),(width,thickness+.08*i,.08),stone)
    elif name=='curb':
        box('curb',(0,0,.10),(width,.4,.2),stone)
    elif name=='stairs':
        for i in range(8):
            box('step',(0,(i+.5)*.5-2,(i+1)*.125),(2,.5,(i+1)*.25),stone)
    elif name=='fence':
        for i in range(9):
            box('upright',(-width/2+i*width/8,0,.9),(.045,.06,1.8),metal)
        for z in (.3,1.5):
            box('rail',(0,0,z),(width,.07,.07),metal)
    elif name=='barrier':
        box('foot',(0,0,.1),(2,.7,.2),stone)
        box('body',(0,0,.65),(2,.4,1.1),stone)
        for x in (-.65,.65):
            box('marker',(x,-.21,.8),(.12,.015,.2),paint)
    elif name=='crate':
        box('body',(0,0,.5),(1,1,1),wood)
        for x in (-.4,.4):
            for y in (-.52,.52):
                box('brace',(x,y,.5),(.10,.05,1.04),wood)
    else:
        box('post',(0,0,1.2),(.08,.08,2.4),metal)
        box('sign',(0,0,2.1),(1.6,.08,.5),paint)
        # Seeded rule-built markings, not externally generated imagery or a logo.
        for i in range(5):
            box('mark',(-.6+i*.28,-.045,2.1),(.10+rng.random()*.06,.01,.25),stone)
    objects=[o for o in scene.objects if o.type=='MESH']
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.object.join()
    obj=bpy.context.object
    obj.name=name
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    bevel=obj.modifiers.new('edge bevel','BEVEL')
    bevel.width=.015
    bevel.segments=1
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.025)
    bpy.ops.object.mode_set(mode='OBJECT')
    output=args.out/name
    output.mkdir(parents=True)
    image=bpy.data.images.new('baked',width=128,height=128,alpha=True)
    for material in obj.data.materials:
        texture=material.node_tree.nodes.new('ShaderNodeTexImage')
        texture.image=image
        material.node_tree.nodes.active=texture
    bpy.ops.object.bake(type='DIFFUSE',pass_filter={'COLOR'},margin=2,use_clear=True)
    image.filepath_raw=str(output/'baked.png')
    image.file_format='PNG'
    image.save()
    for material in obj.data.materials:
        nodes=material.node_tree.nodes
        material.node_tree.links.new(nodes.active.outputs['Color'],nodes.get('Principled BSDF').inputs['Base Color'])
    bpy.ops.export_scene.gltf(filepath=str(output/(name+'.gltf')),export_format='GLTF_SEPARATE',use_selection=True,
                              export_animations=False,export_yup=True,export_apply=True,export_extras=False)
    bpy.ops.wm.obj_export(filepath=str(output/(name+'.obj')),export_selected_objects=True,export_materials=False,
                          global_scale=32,forward_axis='Y',up_axis='Z')
    bounds=[[min(v.co[k] for v in obj.data.vertices)*32 for k in range(3)],
            [max(v.co[k] for v in obj.data.vertices)*32 for k in range(3)]]
    high=sum(len(p.vertices)-2 for p in obj.data.polygons)
    modifier=obj.modifiers.new('distance LOD','DECIMATE')
    modifier.ratio=.5
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    low=sum(len(p.vertices)-2 for p in obj.data.polygons)
    assert 0<low<high, 'LOD must reduce triangle count'
    bpy.ops.export_scene.gltf(filepath=str(output/(name+'_lod1.gltf')),export_format='GLTF_SEPARATE',use_selection=True,
                              export_animations=False,export_yup=True,export_apply=True,export_extras=False)
    report.append(dict(name=name,bounds=bounds,triangles=[high,low]))
(args.out/'kit.json').write_text(json.dumps(dict(version=1,modules=report),sort_keys=True,indent=2)+'\n')
