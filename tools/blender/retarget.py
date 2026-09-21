"""Run in pinned Blender: transfer a supplied clip to a supplied rig, no art generation."""
import argparse
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Matrix,Vector

parser=argparse.ArgumentParser()
parser.add_argument('--parameters',type=Path,required=True)
parser.add_argument('--assets',type=Path,required=True)
parser.add_argument('--out',type=Path,required=True)
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
params=json.loads(args.parameters.read_text())
assert bpy.app.version==(5,0,1), 'Blender 5.0.1 required'
assert type(params['fps']) is int and 1<=params['fps']<=120, 'fps must be 1..120'
scale=params.get('root_motion_scale',1)
assert isinstance(scale,(int,float)) and math.isfinite(scale) and 0<=scale<=100, 'invalid root-motion scale'
assert isinstance(params['bones'],dict) and 1<=len(params['bones'])<=256, 'explicit bone map is required'
bpy.ops.wm.read_factory_settings(use_empty=True)
scene=bpy.context.scene
scene.render.fps=params['fps']


def load(name):
    before=set(scene.objects)
    bpy.ops.import_scene.gltf(filepath=str(args.assets/params[name]),import_pack_images=True)
    objects=set(scene.objects)-before
    rigs=[o for o in objects if o.type=='ARMATURE']
    assert len(rigs)==1, name+' must contain exactly one supplied rig'
    rig=rigs[0]
    assert all(abs(a-b)<1e-5 for ra,rb in zip(rig.matrix_world,Matrix.Identity(4)) for a,b in zip(ra,rb)), 'apply rig object transforms before supplying glTF'
    return rig,objects


source,source_objects=load('clip')
assert source.animation_data, 'supplied clip has no animation'
tracks={track.name:strip for track in source.animation_data.nla_tracks for strip in track.strips}
assert params['animation'] in tracks, 'named animation is absent from supplied clip'
strip=tracks[params['animation']]
action,slot=strip.action,strip.action_slot
begin,end=action.frame_range
assert 0<end-begin<=params['fps']*300, 'clip duration must be in (0,300] seconds'
source.animation_data.action=action
source.animation_data.action_slot=slot
for track in source.animation_data.nla_tracks:
    track.mute=True
# Keep the supplied clip pointers before importing another file with similar names.
target,target_objects=load('rig')
mapping=params['bones']
assert set(mapping)=={b.name for b in target.data.bones}, 'every target bone needs an explicit mapping'
assert all(name in source.data.bones for name in mapping.values()), 'mapped source bone is absent'
assert len(set(mapping.values()))==len(mapping), 'source bone mapping must be one-to-one'
roots=[b for b in target.data.bones if b.parent is None]
assert len(roots)==1 and source.data.bones[mapping[roots[0].name]].parent is None, 'map the single target root to a source root'
for obj in target_objects:
    obj.animation_data_clear()
target.animation_data_create()
for old in bpy.data.actions:
    old.name='supplied_'+old.name
result=bpy.data.actions.new(params['animation'])
target.animation_data.action=result
bones=list(target.pose.bones)
assert all(b.parent is None or bones.index(b.parent)<i for i,b in enumerate(bones)), 'target bones must be parent ordered'
for bone in bones:
    bone.rotation_mode='QUATERNION'
frames=math.ceil(end-begin)+1
scene.frame_start=1
scene.frame_end=frames
max_rotation,max_offset=0,0
root_positions=[]
rotations={b.name:[] for b in bones}
for frame in range(frames):
    source_frame=begin+(end-begin)*frame/(frames-1)
    scene.frame_set(math.floor(source_frame),subframe=source_frame%1)
    desired={}
    for bone in bones:
        source_bone=source.pose.bones[mapping[bone.name]]
        assert all(abs(v-1)<1e-4 for v in source_bone.matrix.to_scale()), 'scaled clip bones are unsupported; supply rotation/root-motion clips'
        source_rest=source_bone.bone.matrix_local
        target_rest=bone.bone.matrix_local
        rotation=(source_bone.matrix.to_quaternion() @ source_rest.to_quaternion().inverted()) @ target_rest.to_quaternion()
        if bone.parent:
            parent_rest=bone.parent.bone.matrix_local
            location=desired[bone.parent.name] @ parent_rest.inverted() @ target_rest.translation
        else:
            location=target_rest.translation+(source_bone.matrix.translation-source_rest.translation)*scale
            root_positions.append(list(location))
        desired[bone.name]=Matrix.LocRotScale(location,rotation,Vector((1,1,1)))
        bone.matrix=desired[bone.name]
        bpy.context.view_layer.update()
        max_rotation=max(max_rotation,rotation.rotation_difference(bone.matrix.to_quaternion()).angle)
        max_offset=max(max_offset,(bone.matrix.translation-location).length)
        rotations[bone.name].append(list(rotation))
        for path in ('location','rotation_quaternion','scale'):
            bone.keyframe_insert(data_path=path,frame=frame+1,group=bone.name)
# Export only the provided destination meshes/skin and this one baked action.
bpy.ops.object.select_all(action='DESELECT')
for obj in target_objects:
    obj.select_set(True)
bpy.context.view_layer.objects.active=target
scene.frame_set(1)
bpy.ops.export_scene.gltf(filepath=str(args.out/'retargeted.gltf'),export_format='GLTF_SEPARATE',use_selection=True,
                          export_animations=True,export_animation_mode='ACTIVE_ACTIONS',export_frame_range=True,
                          export_force_sampling=True,export_yup=True,export_extras=False,
                          export_nla_strips_merged_animation_name=params['animation'])
report=dict(version=1,animation=params['animation'],bones=mapping,frames=frames,fps=params['fps'],
            max_rotation_error_radians=max_rotation,max_bind_offset_error=max_offset,
            root_displacement=math.dist(root_positions[0],root_positions[-1]),
            animated_bones=sum(any(math.dist(values[0],v)>1e-5 for v in values) for values in rotations.values()),
            method='supplied global rest-to-pose rotations; target bind offsets preserved; explicit root-motion scale')
(args.out/'retarget-report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
