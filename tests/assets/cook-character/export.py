"""Explicit fixture authoring: Blender 4.5.3 --background --python export.py -- OUTPUT."""
import math
from pathlib import Path
import sys

import bpy

output = Path(sys.argv[sys.argv.index('--') + 1]).resolve()
output.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.render.fps = 30
scene.frame_start, scene.frame_end = 1, 31
image = bpy.data.images.new('character', width=16, height=16, alpha=True)
pixels = []
for y in range(16):
    for x in range(16):
        pixels.extend((0.15, 0.4, 0.9, 1) if (x // 4 + y // 4) % 2 else (0.95, 0.65, 0.12, 1))
image.pixels = pixels
image.filepath_raw = str(output / 'character.png')
image.file_format = 'PNG'
image.save()
material = bpy.data.materials.new('character')
material.use_nodes = True
surface = material.node_tree.nodes.get('Principled BSDF')
surface.inputs['Roughness'].default_value = 1
texture = material.node_tree.nodes.new('ShaderNodeTexImage')
texture.image = image
material.node_tree.links.new(texture.outputs['Color'], surface.inputs['Base Color'])

armature = bpy.data.armatures.new('character_skeleton')
rig = bpy.data.objects.new('character', armature)
scene.collection.objects.link(rig)
bpy.context.view_layer.objects.active = rig
rig.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
for name, head, tail in [('root', (0, 0, 0), (0, 0, 1)),
                         ('arm.L', (-0.3, 0, 1.3), (-0.7, 0, 1.3)),
                         ('arm.R', (0.3, 0, 1.3), (0.7, 0, 1.3))]:
    bone = armature.edit_bones.new(name)
    bone.head, bone.tail = head, tail
    if name != 'root':
        bone.parent = armature.edit_bones['root']
bpy.ops.object.mode_set(mode='OBJECT')
for name, position, size, bone in [
        ('torso', (0, 0, 1.15), (0.55, 0.3, 0.6), 'root'),
        ('head', (0, 0, 1.65), (0.3, 0.3, 0.35), 'root'),
        ('leg.L', (-0.16, 0, 0.4), (0.22, 0.28, 0.8), 'root'),
        ('leg.R', (0.16, 0, 0.4), (0.22, 0.28, 0.8), 'root'),
        ('arm_mesh.L', (-0.5, 0, 1.3), (0.45, 0.2, 0.2), 'arm.L'),
        ('arm_mesh.R', (0.5, 0, 1.3), (0.45, 0.2, 0.2), 'arm.R')]:
    bpy.ops.mesh.primitive_cube_add(size=1, location=position)
    mesh = bpy.context.object
    mesh.name, mesh.dimensions = name, size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    mesh.data.materials.append(material)
    group = mesh.vertex_groups.new(name=bone)
    group.add(list(range(len(mesh.data.vertices))), 1, 'REPLACE')
    modifier = mesh.modifiers.new('skeleton', 'ARMATURE')
    modifier.object = rig
    mesh.parent = rig

rig.animation_data_create()
for name in ('idle', 'wave'):
    action = bpy.data.actions.new(name)
    rig.animation_data.action = action
    for pose in rig.pose.bones:
        pose.location = (0, 0, 0)
        pose.rotation_mode = 'XYZ'
        pose.rotation_euler = (0, 0, 0)
    for frame, factor in [(1, 0), (16, 1), (31, 0)]:
        if name == 'idle':
            pose = rig.pose.bones['root']
            pose.location.z = factor * 0.04
            pose.keyframe_insert('location', frame=frame, group='root')
        else:
            pose = rig.pose.bones['arm.L']
            pose.rotation_euler.z = factor * math.radians(65)
            pose.keyframe_insert('rotation_euler', frame=frame, group='arm.L')
    action.use_fake_user = True
rig.animation_data.action = None
for pose in rig.pose.bones:
    pose.location = (0, 0, 0)
    pose.rotation_euler = (0, 0, 0)
scene.frame_set(1)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.gltf(filepath=str(output / 'character.gltf'), export_format='GLTF_SEPARATE',
                          use_selection=True, export_yup=True, export_animations=True,
                          export_animation_mode='ACTIONS', export_force_sampling=True,
                          export_skins=True, export_cameras=False, export_lights=False)
