"""Original articulated civilian, authored in metres, facing +X.

Exports joint-local GLBs and an editable assembly. No downloaded character assets.
Run: blender --background --python Tools/Blender/build_hider.py
"""
from pathlib import Path
import math
import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Assets' / 'Hider'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)

def material(name, rgb, rough=.7):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*rgb, 1)
    m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*rgb, 1)
    p.inputs['Roughness'].default_value = rough
    return m

TEAL = material('Hider_Cotton', (.075, .29, .29))
RIB = material('Hider_RibKnit', (.035, .15, .16))
PANTS = material('Hider_Twill', (.065, .085, .115))
SEAM = material('Hider_Stitch', (.15, .20, .23))
SKIN = material('Hider_Skin', (.64, .36, .22), .78)
HAIR = material('Hider_Hair', (.025, .018, .016))
SOLE = material('Hider_Rubber', (.68, .69, .61))
SHOE = material('Hider_Suede', (.22, .25, .22))
CREAM = material('Hider_Cord', (.8, .74, .59))
EYE = material('Hider_Eye', (.012, .018, .018), .3)

parts = {}
active = []
def finish(obj, mat):
    obj.data.materials.append(mat)
    for p in obj.data.polygons:
        p.use_smooth = True
    active.append(obj)
    return obj

def loft(name, rings, mat, sides=16):
    # Each ring is (z, forward centre, x radius, y radius).
    verts = [(x + rx*math.cos(i*2*math.pi/sides), ry*math.sin(i*2*math.pi/sides), z)
             for z,x,rx,ry in rings for i in range(sides)]
    faces = [tuple(range(sides-1,-1,-1))]
    for j in range(len(rings)-1):
        for i in range(sides):
            k=j*sides+i; n=j*sides+(i+1)%sides
            faces.append((k,n,n+sides,k+sides))
    faces.append(tuple((len(rings)-1)*sides+i for i in range(sides)))
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(verts,[],faces); mesh.update()
    ob=bpy.data.objects.new(name,mesh); bpy.context.collection.objects.link(ob)
    return finish(ob,mat)

def ell(name, pos, scale, mat):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=20, ring_count=12, location=pos)
    ob=bpy.context.object; ob.name=name; ob.scale=scale
    return finish(ob,mat)

def box(name,pos,scale,mat,bevel=.012):
    bpy.ops.mesh.primitive_cube_add(size=1,location=pos)
    ob=bpy.context.object; ob.name=name; ob.dimensions=scale
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    m=ob.modifiers.new('Soft tailored edge','BEVEL'); m.width=bevel; m.segments=3
    bpy.ops.object.modifier_apply(modifier=m.name)
    return finish(ob,mat)

def cord(name, points, radius, mat):
    c=bpy.data.curves.new(name,'CURVE'); c.dimensions='3D'; c.bevel_depth=radius; c.bevel_resolution=2
    s=c.splines.new('POLY'); s.points.add(len(points)-1)
    for p,co in zip(s.points,points): p.co=(*co,1)
    ob=bpy.data.objects.new(name,c); bpy.context.collection.objects.link(ob)
    bpy.context.view_layer.objects.active=ob; ob.select_set(True)
    bpy.ops.object.convert(target='MESH')
    ob=bpy.context.object
    return finish(ob,mat)

def save_part(name, at):
    bpy.ops.object.select_all(action='DESELECT')
    for ob in active: ob.select_set(True)
    bpy.context.view_layer.objects.active=active[0]
    bpy.ops.object.join()
    ob=bpy.context.object; ob.name='SM_Hider_'+name
    bpy.context.scene.cursor.location=(0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    bpy.ops.export_scene.gltf(filepath=str(OUT/(ob.name+'.glb')),export_format='GLB',use_selection=True)
    ob.location=at; parts[name]=ob; active.clear()

# Torso rises from the waist. Layered hem, raglan seams and kangaroo pocket.
loft('Hoodie',[(0,0,.125,.155),(.055,0,.135,.17),(.24,0,.142,.18),(.40,-.008,.14,.225),(.47,-.015,.105,.17),(.50,-.02,.07,.085)],TEAL)
loft('Hem',[(-.015,0,.127,.156),(.03,0,.132,.162)],RIB)
box('Pocket',(.136,0,.15),(.018,.235,.10),RIB,.025)
cord('Pocket seam',[(.147,-.09,.175),(.148,-.06,.19),(.148,.06,.19),(.147,.09,.175)],.0025,TEAL)
ell('Folded hood',(-.075,0,.455),(.103,.146,.082),RIB)
loft('Neck',[(.46,-.005,.054,.06),(.57,-.005,.051,.057)],SKIN)
for s in [-1,1]:
    cord('Hood drawstring',[(.096,s*.05,.463),(.144,s*.053,.395),(.149,s*.048,.335)],.0035,CREAM)
    box('Cord tip',(.149,s*.048,.327),(.009,.009,.022),SOLE,.002)
    cord('Raglan seam',[(.06,s*.075,.485),(.11,s*.15,.432),(.115,s*.185,.35)],.002, RIB)
box('Chest badge',(.143,-.087,.34),(.007,.047,.029),CREAM,.004)
save_part('Torso',(0,0,.89))

loft('Hips',[(-.13,0,.098,.135),(-.04,0,.125,.152),(.04,0,.12,.152)],PANTS)
cord('Waist seam',[(.124,-.11,0),(.127,0,0),(.124,.11,0)],.003,SEAM)
save_part('Pelvis',(0,0,.89))

# A deliberately stylized face: jaw, nose bridge, brow, ears, eyelids and hair cap.
loft('Face',[(0,.009,.045,.043),(.035,.022,.065,.075),(.105,.005,.082,.087),(.18,-.008,.083,.084),(.215,-.014,.065,.067),(.232,-.019,.027,.037)],SKIN,24)
ell('Hair crown',(-.03,0,.194),(.087,.092,.06),HAIR)
ell('Back hair',(-.061,0,.139),(.048,.087,.086),HAIR)
for s in [-1,1]:
    ell('Ear',(-.005,s*.087,.105),(.022,.015,.035),SKIN)
    ell('Ear inset',(.01,s*.098,.107),(.008,.006,.019),RIB)
    ell('Eye',(.081,s*.036,.126),(.005,.012,.006),EYE)
    cord('Brow',[(.079,s*.021,.15),(.08,s*.038,.154),(.069,s*.053,.15)],.003,HAIR)
    ell('Sideburn',(-.007,s*.079,.163),(.023,.011,.037),HAIR)
loft('Nose',[(.077,.083,.012,.015),(.094,.101,.015,.012),(.145,.079,.005,.008)],SKIN,10)
cord('Mouth',[(.077,-.023,.059),(.085,0,.056),(.077,.023,.059)],.002,HAIR)
for i in range(5):
    ell('Swept fringe',(.049-i*.004,-.065+i*.029,.197+i*.004),(.044,.024,.024),HAIR)
save_part('Head',(0,0,1.445))

loft('Upper sleeve',[(-.285,0,.061,.061),(-.24,.003,.07,.073),(-.08,0,.08,.08),(.022,0,.067,.078)],TEAL)
cord('Sleeve seam',[(0,.078,-.04),(.005,.072,-.15),(0,.061,-.275)],.002,RIB)
save_part('UpperArm',(0,.219,1.305))
loft('Fore sleeve',[(-.245,0,.043,.044),(-.20,.006,.049,.049),(-.09,.007,.061,.062),(.012,0,.061,.061)],TEAL)
loft('Cuff',[(-.271,0,.04,.042),(-.232,0,.044,.045)],RIB)
save_part('Forearm',(0,.219,1.025))
ell('Palm',(0,0,-.043),(.025,.04,.048),SKIN)
for i in range(4):
    y=-.027+i*.018; length=[.053,.063,.060,.045][i]
    ob=ell('Finger',(.007,y,-.077-length*.35),(.013,.010,length*.6),SKIN)
    ob.rotation_euler[1]=-.13
ell('Thumb',(.024,-.036,-.033),(.018,.016,.034),SKIN)
save_part('Hand',(0,.219,.755))

loft('Trouser thigh',[(-.425,0,.069,.066),(-.35,0,.074,.070),(-.17,0,.083,.085),(.02,0,.092,.095)],PANTS)
cord('Side seam',[(0,.092,-.05),(0,.078,-.23),(0,.067,-.41)],.0025,SEAM)
box('Pocket welt',(.068,.049,-.07),(.035,.009,.085),SEAM,.004)
save_part('Thigh',(0,.099,.89))
loft('Trouser calf',[(-.401,0,.050,.048),(-.36,0,.052,.05),(-.19,-.012,.070,.065),(-.06,0,.064,.062),(.012,0,.067,.064)],PANTS)
loft('Turned cuff',[(-.414,0,.052,.05),(-.385,0,.054,.052)],SEAM)
save_part('Shin',(0,.099,.47))
box('Sole',(.043,0,-.038),(.251,.117,.032),SOLE,.018)
ell('Shoe upper',(.043,0,-.007),(.122,.057,.044),SHOE)
box('Heel',(-.055,0,.009),(.043,.103,.044),RIB,.009)
box('Tongue',(.016,0,.033),(.097,.057,.016),PANTS,.006)
for x in [-.01,.012,.034,.055]:
    cord('Lace',[(x,-.025,.039),(x+.006,0,.044),(x,.025,.039)],.003,CREAM)
save_part('Shoe',(0,.099,.065))

# Assembly with named joints and editable, linked right-side counterparts.
for name in ['UpperArm','Forearm','Hand','Thigh','Shin','Shoe']:
    ob=parts[name]; cp=ob.copy(); cp.data=ob.data
    bpy.context.collection.objects.link(cp); cp.name=ob.name+'_R'; cp.location.y=-ob.location.y

# Studio preview. Environment and light sources are excluded from part exports.
floor=box('Studio',(0,0,-.035),(200,200,.04),material('Studio',(.055,.068,.080)),.005)
active.clear()
def area(name,pos,power,size):
    bpy.ops.object.light_add(type='AREA',location=pos)
    ob=bpy.context.object; ob.name=name; ob.data.energy=power; ob.data.shape='DISK'; ob.data.size=size
    ob.rotation_euler=(Vector((0,0,.9))-ob.location).to_track_quat('-Z','Y').to_euler()
area('Key',(3,-4,5),650,4)
area('Fill',(2,3,2.5),400,3)
area('Rim',(-3,1,3),850,2)
bpy.ops.object.camera_add(location=(3.15,-3.6,2.05))
cam=bpy.context.object; cam.rotation_euler=(Vector((0,0,.88))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO'; cam.data.ortho_scale=2.12
scene=bpy.context.scene; scene.camera=cam
scene.render.engine='CYCLES'; scene.cycles.samples=32
scene.render.resolution_x=880; scene.render.resolution_y=1040; scene.render.resolution_percentage=100
scene.world.color=(.2,.2,.2)
scene.view_settings.view_transform='AgX'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Trinity_Hider.blend'))
scene.render.filepath=str(OUT/'Trinity_Hider_Studio.png')
bpy.ops.render.render(write_still=True)
print('[HIDER_MODEL] complete: 9 joint-local meshes, editable source and studio preview')
