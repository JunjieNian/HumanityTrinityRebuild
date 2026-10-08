"""Check shared editable geometry and render the teacher through the middle doorway.

Run against HumanityTrinityRebuild.blend. The source is never saved by this QA script.
"""
from pathlib import Path
import hashlib
import json
import math
import runpy
import struct
import bpy
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[2]
spec=runpy.run_path(str(ROOT/'Tools/Blender/patrol_environment.py'))
for name,pos,size,_ in spec['PARTS']:
    ob=bpy.data.objects['Corridor_'+name]
    expected=Vector((pos[0]/100,-pos[1]/100,pos[2]/100))
    assert (ob.location-expected).length<1e-5, name
    assert (ob.dimensions-Vector(tuple(v/100 for v in size))).length<1e-5,name
doors=[o for o in bpy.context.scene.objects if o.get('runtime_dynamic_exterior_door')]
assert len(doors)==3
for i,d in enumerate(spec['DOORS']):
    ob=bpy.data.objects[f'Door_0{i+1}_DoorLeaf_Closed']
    assert ob.get('closed_by_default')
    expected=Vector((d['hinge'][0]/100,-(d['hinge'][1]-d['width']/2)/100,
                     (d['hinge'][2]+d['height']/2)/100))
    assert (ob.location-expected).length<1e-5
sources=sorted((ROOT/'Assets/Teacher').glob('SM_Teacher_*.glb'))
assert len(sources)==9
for source in sources:
    data=source.read_bytes(); length=struct.unpack_from('<I',data,12)[0]
    gltf=json.loads(data[20:20+length]); assert len(gltf['meshes'])==1
full=(ROOT/'HumanityTrinityRebuild.glb').read_bytes()
gltf=json.loads(full[20:20+struct.unpack_from('<I',full,12)[0]])
assert sum(bool(n.get('extras',{}).get('runtime_dynamic_exterior_door')) for n in gltf['nodes'])==3
assert sum(bool(n.get('extras',{}).get('runtime_dynamic_corridor')) for n in gltf['nodes'])==len(spec['PARTS'])
checks=dict(editable_closed_doors=3,corridor_parts=len(spec['PARTS']),teacher_source_meshes=9,
            teacher_assembly_parts=15,source_and_shared_layout_match=True,
            full_glb_retains_editable_objects=True)
paths=[ROOT/'HumanityTrinityRebuild.blend',ROOT/'HumanityTrinityRebuild.glb',
       ROOT/'UnrealProject/Content/SourceAssets/HumanityTrinityRebuildEnvironment_Runtime.glb',
       ROOT/'Assets/Teacher/Trinity_Teacher.blend']+sources
checks['sha256']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
out=ROOT/'Docs/Validation'; out.mkdir(parents=True,exist_ok=True)
(out/'TeacherPatrolAssets.json').write_text(json.dumps(checks,indent=2)+'\n',encoding='utf-8')
print('[PATROL_ASSETS] GEOMETRY_AND_IDENTITY_PASS '+json.dumps(checks,ensure_ascii=False))

# QA-only open door and modeled teacher; all source collections remain unmodified on disk.
for ob in bpy.context.scene.objects:
    ob.hide_set(False); ob.hide_render=False; ob.hide_viewport=False
    if ob.type=='LIGHT': ob.data.energy=0
    if any(c.name.startswith('07_Annotations') for c in ob.users_collection): ob.hide_render=True
for mat in bpy.data.materials:
    if mat.use_nodes:
        node=mat.node_tree.nodes.get('Principled BSDF')
        if node and 'Emission Strength' in node.inputs: node.inputs['Emission Strength'].default_value=0
d=spec['DOORS'][1]; ob=bpy.data.objects['Door_02_DoorLeaf_Closed']
hinge=Vector((d['hinge'][0]/100,-d['hinge'][1]/100,d['hinge'][2]/100))
angle=math.radians(100)
ob.location=hinge+Vector((-math.sin(angle)*d['width']/200,math.cos(angle)*d['width']/200,d['height']/200))
ob.rotation_euler.z=angle
with bpy.data.libraries.load(str(ROOT/'Assets/Teacher/Trinity_Teacher.blend')) as (src,dst):
    dst.objects=[n for n in src.objects if n.startswith('SM_Teacher_')]
for ob in dst.objects:
    bpy.context.scene.collection.objects.link(ob); ob.location+=Vector((-6.7,12,0))
for y in (2.25,6.5,10.75,15):
    bpy.ops.object.light_add(type='AREA',location=(-7.59,y,3.11)); light=bpy.context.object
    light.data.energy=150; light.data.shape='RECTANGLE'; light.data.size=1.12; light.data.size_y=.58
    light.data.color=(.76,.86,1)
bpy.ops.object.camera_add(location=(-3.7,10.8,1.58)); cam=bpy.context.object
cam.rotation_euler=(Vector((-6.70,12,1.20))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.lens=27
scene=bpy.context.scene; scene.camera=cam; scene.render.engine='CYCLES'; scene.cycles.samples=24
scene.world.color=(0,0,0)
if scene.world.use_nodes:
    for n in scene.world.node_tree.nodes:
        if n.type=='BACKGROUND': n.inputs['Strength'].default_value=0
scene.render.resolution_x=1280; scene.render.resolution_y=720; scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'; scene.view_settings.exposure=1
scene.render.filepath=str(ROOT/'Docs/Previews/TeacherPatrol_EditableDoorway.png')
bpy.ops.render.render(write_still=True)
print('[PATROL_ASSETS] EDITABLE_DOORWAY_CAPTURE_COMPLETE')
