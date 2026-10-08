"""Original editable patrol teacher: tailored blazer, tie, glasses, ID and leather shoes.

Nine joint-local assets, fifteen-part assembly, facing +X, height 1.82 metres.
Run: blender --background --python Tools/Blender/build_teacher.py
"""
from pathlib import Path
import math
import bpy
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'Assets'/'Teacher'; OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
def material(name,c,rough=.65,metal=0):
    m=bpy.data.materials.new('Teacher_'+name); m.diffuse_color=(*c,1); m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value=(*c,1)
    p.inputs['Roughness'].default_value=rough; p.inputs['Metallic'].default_value=metal
    return m
JACKET=material('NavyWool',(.038,.065,.092)); LAPEL=material('Lapel',(.06,.095,.13))
SHIRT=material('CottonShirt',(.77,.80,.75)); TIE=material('BurgundyTie',(.22,.028,.025))
PANTS=material('CharcoalTwill',(.055,.065,.08)); SKIN=material('Skin',(.60,.37,.24),.78)
HAIR=material('SaltAndPepperHair',(.10,.105,.11)); GREY=material('GreyTemples',(.31,.32,.31))
SHOE=material('BlackLeather',(.026,.021,.020),.3); SOLE=material('Rubber',(.012,.014,.017))
METAL=material('GlassesMetal',(.22,.24,.25),.28,.7); EYE=material('Eye',(.014,.013,.013),.3)
ID=material('StaffCard',(.84,.80,.67)); INK=material('CardInk',(.05,.13,.20))
active=[]; parts={}
def finish(o,m,smooth=True):
    o.data.materials.append(m)
    for p in o.data.polygons: p.use_smooth=smooth
    active.append(o); return o
def ell(name,p,s,m):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=16,location=p)
    o=bpy.context.object; o.name=name; o.scale=s; return finish(o,m)
def box(name,p,s,m,bevel=.008):
    bpy.ops.mesh.primitive_cube_add(size=1,location=p); o=bpy.context.object; o.name=name; o.dimensions=s
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    b=o.modifiers.new('Tailored rounded edges','BEVEL'); b.width=bevel; b.segments=3
    bpy.ops.object.modifier_apply(modifier=b.name); return finish(o,m)
def loft(name,rings,m,sides=24):
    verts=[(x+rx*math.cos(i*math.tau/sides),ry*math.sin(i*math.tau/sides),z)
           for z,x,rx,ry in rings for i in range(sides)]
    faces=[tuple(range(sides-1,-1,-1))]
    for j in range(len(rings)-1):
        for i in range(sides):
            k=j*sides+i; n=j*sides+(i+1)%sides; faces.append((k,n,n+sides,k+sides))
    faces.append(tuple((len(rings)-1)*sides+i for i in range(sides)))
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(verts,[],faces); mesh.update()
    o=bpy.data.objects.new(name,mesh); bpy.context.collection.objects.link(o); return finish(o,m)
def cord(name,points,r,m):
    curve=bpy.data.curves.new(name,'CURVE'); curve.dimensions='3D'; curve.bevel_depth=r; curve.bevel_resolution=3
    spl=curve.splines.new('POLY'); spl.points.add(len(points)-1)
    for p,co in zip(spl.points,points): p.co=(*co,1)
    o=bpy.data.objects.new(name,curve); bpy.context.collection.objects.link(o)
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active=o
    bpy.ops.object.convert(target='MESH'); return finish(bpy.context.object,m)
def patch(name,points,m):
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(points,[],[tuple(range(len(points)))]); mesh.update()
    o=bpy.data.objects.new(name,mesh); bpy.context.collection.objects.link(o)
    b=o.modifiers.new('Cloth thickness','SOLIDIFY'); b.thickness=.004
    return finish(o,m,False)
def save_part(name,at):
    bpy.ops.object.select_all(action='DESELECT')
    for o in active: o.select_set(True)
    bpy.context.view_layer.objects.active=active[0]; bpy.ops.object.convert(target='MESH'); bpy.ops.object.join()
    o=bpy.context.object; o.name='SM_Teacher_'+name; bpy.context.scene.cursor.location=(0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR'); bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    bpy.ops.export_scene.gltf(filepath=str(OUT/(o.name+'.glb')),export_format='GLB',use_selection=True)
    o.location=at; parts[name]=o; active.clear()

# Jacket and crisp shirt, distinct from the student's casual hoodie.
loft('Blazer',[(-.08,0,.135,.17),(.02,0,.14,.18),(.22,0,.152,.185),(.40,-.006,.15,.224),(.47,-.015,.105,.19),(.51,-.025,.065,.08)],JACKET)
patch('Shirt front',[(.143,-.066,.40),(.143,.066,.40),(.15,.044,.17),(.15,-.044,.17)],SHIRT)
for s in [-1,1]:
    patch('Notched lapel',[(.137,s*.056,.46),(.167,s*.142,.35),(.159,s*.089,.30),(.157,s*.116,.265),(.15,s*.035,.15)],LAPEL)
    patch('White collar',[(.143,s*.027,.47),(.16,s*.074,.418),(.165,s*.032,.38),(.16,0,.434)],SHIRT)
    box('Welt pocket',(.14,s*.119,.08),(.018,.075,.017),LAPEL,.002)
    cord('Jacket seam',[(.112,s*.16,-.068),(.13,s*.164,.05),(.144,s*.168,.19)],.0017,LAPEL)
patch('Tie blade',[(.173,-.018,.40),(.173,.018,.40),(.178,.024,.22),(.178,0,.185),(.178,-.024,.22)],TIE)
ell('Tie knot',(.175,0,.414),(.011,.023,.021),TIE)
for z in (.14,.025): ell('Jacket button',(.148,0,z),(.006,.009,.009),METAL)
loft('Neck',[(.455,-.013,.052,.06),(.60,-.013,.053,.06)],SKIN)
box('Breast pocket',(.134,-.12,.338),(.012,.075,.013),LAPEL,.001)
cord('ID lanyard',[(.094,.068,.47),(.17,.075,.38),(.167,.09,.29)],.003,INK)
box('Staff badge',(.161,.09,.265),(.012,.062,.085),ID,.004)
box('Badge portrait',(.17,.076,.277),(.003,.022,.028),INK,.001)
for z in (.26,.249,.238): box('Badge text',(.17,.094,z),(.003,.040,.003),INK,.0004)
save_part('Torso',(0,0,.94))

loft('Hips',[(-.13,0,.098,.142),(-.04,0,.127,.158),(.04,0,.12,.156)],PANTS)
loft('Belt',[(-.016,0,.129,.16),(.024,0,.129,.16)],SHOE)
box('Belt buckle',(.134,0,.005),(.013,.037,.035),METAL,.003)
save_part('Pelvis',(0,0,.94))

# Sculpted adult features and separate thin eyeglass rims with open lenses.
loft('Face',[(0,.005,.038,.046),(.032,.017,.061,.075),(.095,.006,.081,.089),(.18,-.012,.086,.087),(.23,-.02,.062,.065),(.255,-.025,.027,.034)],SKIN)
ell('Hair crown',(-.035,0,.221),(.087,.094,.058),HAIR)
ell('Hair back',(-.067,0,.152),(.046,.089,.085),HAIR)
for s in [-1,1]:
    ell('Ear',(-.005,s*.089,.117),(.022,.017,.037),SKIN)
    ell('Silver temple',(-.019,s*.081,.185),(.027,.012,.041),GREY)
    ell('Eye white',(.081,s*.037,.139),(.005,.017,.008),SHIRT)
    ell('Iris',(.086,s*.037,.139),(.003,.006,.006),EYE)
    cord('Eyebrow',[(.079,s*.019,.169),(.083,s*.038,.177),(.070,s*.060,.172)],.004,HAIR)
    points=[(.093,s*.039+.026*math.cos(t*math.tau/24),.14+.021*math.sin(t*math.tau/24)) for t in range(25)]
    cord('Spectacle rim',points,.0024,METAL)
    cord('Spectacle arm',[(.093,s*.065,.15),(.055,s*.088,.156),(-.03,s*.094,.15)],.0026,METAL)
    cord('Cheek crease',[(.085,s*.035,.098),(.082,s*.036,.089),(.073,s*.030,.077)],.0012,HAIR)
cord('Glasses bridge',[(.093,-.013,.146),(.106,0,.153),(.093,.013,.146)],.0023,METAL)
loft('Nose',[(.088,.09,.012,.014),(.104,.113,.015,.015),(.165,.080,.005,.008)],SKIN,12)
cord('Mouth',[(.076,-.023,.064),(.086,0,.061),(.077,.023,.064)],.0022,HAIR)
for i in range(7): ell('Side parted hair',(.04-i*.005,-.071+i*.022,.224+i*.002),(.045,.019,.022),HAIR)
save_part('Head',(0,0,1.54))

loft('Upper blazer sleeve',[(-.306,0,.064,.061),(-.23,0,.07,.074),(-.075,0,.082,.085),(.028,0,.070,.080)],JACKET)
save_part('UpperArm',(0,.23,1.40))
loft('Lower blazer sleeve',[(-.277,0,.043,.045),(-.20,.004,.051,.052),(-.06,0,.063,.064),(.015,0,.064,.062)],JACKET)
loft('Shirt cuff',[(-.300,0,.044,.045),(-.273,0,.045,.046)],SHIRT)
for z in (-.248,-.226,-.204): ell('Sleeve button',(.045,-.012,z),(.005,.006,.006),METAL)
save_part('Forearm',(0,.23,1.095))
ell('Palm',(0,0,-.044),(.026,.043,.047),SKIN)
for i in range(4):
    y=-.028+i*.019; length=[.056,.065,.062,.048][i]
    ell('Finger',(.008,y,-.078-length*.35),(.012,.009,length*.6),SKIN)
ell('Thumb',(.025,-.038,-.033),(.018,.016,.036),SKIN)
save_part('Hand',(0,.23,.797))

loft('Trouser thigh',[(-.447,0,.066,.068),(-.35,0,.074,.075),(-.15,0,.084,.088),(.02,0,.091,.098)],PANTS)
cord('Pressed crease',[(.089,0,-.035),(.08,0,-.17),(.068,0,-.43)],.0018,LAPEL)
save_part('Thigh',(0,.105,.94))
loft('Trouser shin',[(-.427,0,.052,.052),(-.36,0,.054,.052),(-.18,-.009,.07,.066),(.015,0,.067,.068)],PANTS)
cord('Lower pressed crease',[(.067,0,-.02),(.056,0,-.21),(.053,0,-.416)],.0018,LAPEL)
save_part('Shin',(0,.105,.497))
box('Leather shoe sole',(.049,0,-.042),(.266,.122,.029),SOLE,.012)
ell('Oxford shoe',(.052,0,-.009),(.13,.059,.046),SHOE)
box('Shoe heel',(-.048,0,-.03),(.065,.115,.046),SHOE,.007)
cord('Toe cap seam',[(.129,-.048,.012),(.13,0,.028),(.129,.048,.012)],.0017,METAL)
for x in (-.013,.006,.025,.044): cord('Laces',[(x,-.025,.032),(x+.005,0,.038),(x,.025,.032)],.0018,SOLE)
save_part('Shoe',(0,.105,.067))

for name in ('UpperArm','Forearm','Hand','Thigh','Shin','Shoe'):
    o=parts[name]; copy=o.copy(); copy.data=o.data; bpy.context.collection.objects.link(copy)
    copy.name=o.name+'_R'; copy.location.y=-o.location.y
floor=box('Studio',(0,0,-.05),(200,200,.04),material('Studio',(.075,.085,.10))); active.clear()
for name,p,power,size in [('Key',(3,-4,5),650,4),('Fill',(2,3,2.5),450,3),('Rim',(-3,1,3),850,2)]:
    bpy.ops.object.light_add(type='AREA',location=p); o=bpy.context.object; o.name=name
    o.data.energy=power; o.data.shape='DISK'; o.data.size=size
    o.rotation_euler=(Vector((0,0,.95))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(3.4,-4.0,2.1)); cam=bpy.context.object
cam.rotation_euler=(Vector((0,0,.91))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO'; cam.data.ortho_scale=2.15
scene=bpy.context.scene; scene.camera=cam; scene.render.engine='CYCLES'; scene.cycles.samples=32
scene.render.resolution_x=880; scene.render.resolution_y=1040; scene.render.resolution_percentage=100
scene.world.color=(.2,.2,.2); scene.view_settings.view_transform='AgX'
scene['asset_description']='Original fictional adult teacher; blazer, ID, glasses; 15 articulated parts from 9 source meshes.'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Trinity_Teacher.blend'))
scene.render.filepath=str(OUT/'Trinity_Teacher_Studio.png'); bpy.ops.render.render(write_still=True)
print('[TEACHER_MODEL] COMPLETE meshes=9 assembly=15 height_m=1.82 editable=YES')
