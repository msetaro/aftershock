"""Author original animation acceptance rigs with Blender 4.5.3; never run in CI."""
import math
from pathlib import Path
import sys

import bpy
from mathutils import Vector

output = Path(sys.argv[sys.argv.index('--') + 1]).resolve()
output.mkdir(parents=True, exist_ok=True)


def skeleton(name, bones):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.fps = 30
    scene.frame_start, scene.frame_end = 1, 31
    armature = bpy.data.armatures.new(name + '_skeleton')
    rig = bpy.data.objects.new(name, armature)
    scene.collection.objects.link(rig)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    for key, parent, head, tail in bones:
        bone = armature.edit_bones.new(key)
        bone.head, bone.tail = head, tail
        if parent:
            bone.parent = armature.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    rig.animation_data_create()
    return rig


def material(name, color):
    value = bpy.data.materials.new(name)
    value.use_nodes = True
    surface = value.node_tree.nodes.get('Principled BSDF')
    surface.inputs['Base Color'].default_value = (*color, 1)
    surface.inputs['Roughness'].default_value = 1
    return value


def block(rig, name, bone, center, size, surface, direction=None):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    mesh = bpy.context.object
    mesh.name, mesh.dimensions = name, size
    if direction is not None:
        mesh.rotation_mode = 'QUATERNION'
        mesh.rotation_quaternion = direction.to_track_quat('Y', 'Z')
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    mesh.data.materials.append(surface)
    group = mesh.vertex_groups.new(name=bone)
    group.add(list(range(len(mesh.data.vertices))), 1, 'REPLACE')
    modifier = mesh.modifiers.new('skeleton', 'ARMATURE')
    modifier.object = rig
    mesh.parent = rig


def limb(rig, bone, width, surface):
    source = rig.data.bones[bone]
    direction = source.tail_local - source.head_local
    block(rig, bone + '_mesh', bone, (source.head_local + source.tail_local) / 2,
          (width, direction.length, width), surface, direction)


def reset(rig):
    for pose in rig.pose.bones:
        pose.location = (0, 0, 0)
        pose.rotation_mode = 'XYZ'
        pose.rotation_euler = (0, 0, 0)
        pose.scale = (1, 1, 1)


def translate(rig, bone, delta):
    pose = rig.pose.bones[bone]
    pose.location = pose.bone.matrix_local.to_quaternion().inverted() @ Vector(delta)


def clips(rig, names, pose_at):
    for name in names:
        action = bpy.data.actions.new(name)
        rig.animation_data.action = action
        for frame in range(1, 32):
            reset(rig)
            pose_at(name, (frame - 1) / 30)
            for pose in rig.pose.bones:
                pose.keyframe_insert('location', frame=frame, group=pose.name)
                pose.keyframe_insert('rotation_euler', frame=frame, group=pose.name)
        action.use_fake_user = True
    rig.animation_data.action = None
    reset(rig)
    bpy.context.scene.frame_set(1)
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.export_scene.gltf(filepath=str(output / (rig.name + '.gltf')),
                              export_format='GLTF_SEPARATE', use_selection=True,
                              export_yup=True, export_animations=True,
                              export_animation_mode='ACTIONS', export_force_sampling=True,
                              export_skins=True, export_cameras=False, export_lights=False)


bones = [('root', None, (0, 0, 0), (0, 0, 0.1)),
         ('rifle', 'root', (0, 0, 0), (0.5, 0, 0)),
         ('magazine', 'rifle', (0.12, 0, -0.05), (0.12, 0, -0.25)),
         ('muzzle', 'rifle', (0.85, 0, 0.04), (0.9, 0, 0.04)),
         ('optic', 'rifle', (0.2, 0, 0.14), (0.3, 0, 0.14)),
         ('grip.L', 'rifle', (0.42, 0.09, -0.02), (0.47, 0.09, -0.02)),
         ('grip.R', 'rifle', (0.03, -0.08, -0.11), (0.08, -0.08, -0.11))]
for side, sign, hand in [('L', 1, (0.42, 0.09, -0.02)), ('R', -1, (0.03, -0.08, -0.11))]:
    shoulder, elbow = (-0.4, sign * 0.3, -0.2), (-0.05, sign * 0.35, -0.3)
    bones += [('upperarm.' + side, 'root', shoulder, elbow),
              ('forearm.' + side, 'upperarm.' + side, elbow, hand),
              ('hand.' + side, 'forearm.' + side, hand, (hand[0] + 0.1, hand[1], hand[2]))]
rifle = skeleton('rifle', bones)
metal = material('rifle_metal', (0.12, 0.16, 0.18))
stock = material('rifle_stock', (0.35, 0.24, 0.12))
arms = material('sleeves', (0.18, 0.38, 0.5))
skin = material('gloves', (0.38, 0.35, 0.25))
for name, bone, center, size, surface in [
        ('receiver', 'rifle', (0.19, 0, 0.02), (0.38, 0.10, 0.12), metal),
        ('barrel', 'rifle', (0.63, 0, 0.04), (0.45, 0.045, 0.045), metal),
        ('stock', 'rifle', (-0.19, 0, 0), (0.36, 0.075, 0.15), stock),
        ('handguard', 'rifle', (0.42, 0, 0.015), (0.25, 0.12, 0.1), stock),
        ('pistol_grip', 'rifle', (0, 0, -0.10), (0.075, 0.08, 0.18), stock),
        ('magazine_mesh', 'magazine', (0.12, 0, -0.16), (0.12, 0.07, 0.20), metal),
        ('optic_mesh', 'optic', (0.23, 0, 0.15), (0.17, 0.045, 0.07), metal)]:
    block(rifle, name, bone, center, size, surface)
for side in ('L', 'R'):
    limb(rifle, 'upperarm.' + side, 0.12, arms)
    limb(rifle, 'forearm.' + side, 0.095, arms)
    limb(rifle, 'hand.' + side, 0.085, skin)


def rifle_pose(name, phase):
    pulse = math.sin(math.pi * phase)
    if name == 'idle':
        translate(rifle, 'root', (0, 0, math.sin(phase * 2 * math.pi) * 0.004))
    elif name == 'ads':
        translate(rifle, 'root', (0.08, 0, 0.08))
    elif name == 'fire':
        kick = max(0, 1 - phase * 5)
        translate(rifle, 'rifle', (-0.045 * kick, 0, 0))
        rifle.pose.bones['rifle'].rotation_euler.x = 0.08 * kick
    elif name == 'reload':
        rifle.pose.bones['rifle'].rotation_euler.x = 0.35 * pulse
        translate(rifle, 'magazine', (0, 0, -0.25 * pulse))
        rifle.pose.bones['forearm.L'].rotation_euler.z = 0.5 * pulse
    elif name == 'sprint':
        translate(rifle, 'root', (-0.05, 0, -0.08 + 0.01 * math.sin(phase * 4 * math.pi)))
        rifle.pose.bones['rifle'].rotation_euler.z = 0.35
    elif name == 'jump':
        translate(rifle, 'root', (0, 0, -0.06 * pulse))


clips(rifle, ('idle', 'ads', 'fire', 'reload', 'sprint', 'jump'), rifle_pose)

bones = [('root', None, (0, 0, 0), (0, 0, 0.1)),
         ('pelvis', 'root', (0, 0, 0.9), (0, 0, 1.05)),
         ('spine', 'pelvis', (0, 0, 1.05), (0, 0, 1.45)),
         ('head', 'spine', (0, 0, 1.45), (0, 0, 1.7))]
for side, sign in [('L', 1), ('R', -1)]:
    hip, knee, ankle = (0, sign * 0.13, 0.9), (0, sign * 0.13, 0.48), (0, sign * 0.13, 0.08)
    shoulder, elbow, wrist = (0, sign * 0.25, 1.4), (0.03, sign * 0.43, 1.15), (0.16, sign * 0.48, 0.93)
    bones += [('thigh.' + side, 'pelvis', hip, knee),
              ('shin.' + side, 'thigh.' + side, knee, ankle),
              ('foot.' + side, 'shin.' + side, ankle, (0.2, sign * 0.13, 0.08)),
              ('upperarm.' + side, 'spine', shoulder, elbow),
              ('forearm.' + side, 'upperarm.' + side, elbow, wrist),
              ('hand.' + side, 'forearm.' + side, wrist, (0.25, sign * 0.48, 0.9))]
body = skeleton('body', bones)
cloth = material('body_cloth', (0.72, 0.32, 0.1))
trousers = material('body_trousers', (0.22, 0.29, 0.23))
skin = material('body_skin', (0.62, 0.45, 0.3))
block(body, 'torso', 'spine', (0, 0, 1.25), (0.28, 0.43, 0.5), cloth)
block(body, 'pelvis_mesh', 'pelvis', (0, 0, 0.95), (0.27, 0.34, 0.22), trousers)
block(body, 'head_mesh', 'head', (0, 0, 1.61), (0.25, 0.26, 0.3), skin)
for side in ('L', 'R'):
    for bone, width, surface in [('thigh', 0.17, trousers), ('shin', 0.14, trousers),
                                  ('foot', 0.13, trousers), ('upperarm', 0.14, cloth),
                                  ('forearm', 0.12, cloth), ('hand', 0.10, skin)]:
        limb(body, bone + '.' + side, width, surface)


def body_pose(name, phase):
    stride = math.sin(phase * 2 * math.pi)
    if name in ('walk', 'run'):
        amount = 0.35 if name == 'walk' else 0.65
        translate(body, 'root', ((0.9 if name == 'walk' else 2.2) * phase, 0, 0))
        for side, sign in [('L', 1), ('R', -1)]:
            body.pose.bones['thigh.' + side].rotation_euler.z = amount * stride * sign
            body.pose.bones['shin.' + side].rotation_euler.z = amount * max(0, -stride * sign)
            body.pose.bones['upperarm.' + side].rotation_euler.z = -amount * stride * sign * 0.5
    elif name in ('aim_up', 'aim_down'):
        body.pose.bones['spine'].rotation_euler.z = 0.45 if name == 'aim_up' else -0.45
    elif name == 'crouch':
        translate(body, 'pelvis', (0, 0, -0.3))
        for side in ('L', 'R'):
            body.pose.bones['thigh.' + side].rotation_euler.z = -0.7
            body.pose.bones['shin.' + side].rotation_euler.z = 1.2
    elif name == 'prone':
        translate(body, 'pelvis', (0, 0, -0.65))
        body.pose.bones['pelvis'].rotation_euler.z = -1.35
    elif name in ('lean_left', 'lean_right'):
        body.pose.bones['spine'].rotation_euler.x = 0.2 if name == 'lean_left' else -0.2
    elif name == 'turn':
        body.pose.bones['root'].rotation_euler.y = phase * math.pi / 2
    elif name == 'idle':
        body.pose.bones['spine'].rotation_euler.x = stride * 0.01


clips(body, ('idle', 'walk', 'run', 'aim_up', 'aim_down', 'crouch', 'prone',
             'lean_left', 'lean_right', 'turn'), body_pose)
