"""Photo-guided, editable details, called by the main generator via runpy.

All coordinates below are in the pre-mirror Blender frame. Photographs establish
appearance, not dimensions. Textures are deterministic original procedural assets;
the private reference photographs are not copied or embedded in the public model.
"""

import numpy as np

DETAIL = make_collection("08_PhotoDetails_照片细化")
OVERHEAD = make_collection("09_CeilingDetails_吊顶设备")
TEXTURES = OUT_DIR / "Assets" / "Textures"
TEXTURES.mkdir(parents=True, exist_ok=True)


def color(key, rgba, roughness):
    shader = MAT[key].node_tree.nodes.get("Principled BSDF")
    shader.inputs["Base Color"].default_value = rgba
    shader.inputs["Roughness"].default_value = roughness


def texture_material(key, rgb, height, repeat, roughness):
    """Portable UV textures (not Blender-only Noise nodes) survive GLB export."""
    mat = MAT[key]
    n = rgb.shape[0]
    shader = mat.node_tree.nodes.get("Principled BSDF")
    shader.inputs["Roughness"].default_value = roughness
    for kind in ("BaseColor", "Normal"):
        if kind == "BaseColor":
            pixels = np.dstack((np.clip(rgb, 0, 1), np.ones((n,n))))
        else:
            dy, dx = np.gradient(height)
            normal = np.dstack((-dx * 9, -dy * 9, np.ones((n,n))))
            normal /= np.linalg.norm(normal, axis=2, keepdims=True)
            pixels = np.dstack((normal * .5 + .5, np.ones((n,n))))
        image = bpy.data.images.new("HumanityTrinityRebuild_" + key + "_" + kind, n, n)
        if kind == "Normal":
            image.colorspace_settings.name = "Non-Color"
        image.pixels.foreach_set(pixels.astype(np.float32).ravel())
        image.filepath_raw = str(TEXTURES / (image.name + ".png"))
        image.file_format = "PNG"
        image.save()
        image.pack()
        node = mat.node_tree.nodes.new("ShaderNodeTexImage")
        node.image = image
        if kind == "BaseColor":
            mat.node_tree.links.new(node.outputs["Color"], shader.inputs["Base Color"])
        else:
            bump = mat.node_tree.nodes.new("ShaderNodeNormalMap")
            bump.inputs["Strength"].default_value = .22
            mat.node_tree.links.new(node.outputs["Color"], bump.inputs["Color"])
            mat.node_tree.links.new(bump.outputs["Normal"], shader.inputs["Normal"])
    mat["uv_repeat_m"] = repeat


rng = np.random.default_rng(20260905)
n = 512
v, u = np.mgrid[0:1:complex(n), 0:1:complex(n)]
grain = np.sin((v * 75 + .4*np.sin(u*12) + .15*np.sin(u*31)) * 2*np.pi)
grain += .42*np.sin((v*190 + .55*np.sin(u*8))*2*np.pi)
noise = rng.normal(0,.009,(n,n))
oak = np.dstack((.68 + .035*grain+noise, .52 + .030*grain+noise, .34 + .022*grain+noise))
texture_material("stage_wood", oak, grain*.04, (2.4,.65), .48)
fleck = rng.normal(0,.006,(n,n))
vinyl = np.dstack((.57+fleck,.64+fleck,.60+fleck))
texture_material("floor", vinyl, fleck*.7, (1.8,1.8), .52)
weave = .5*np.sin(u*256*np.pi)*np.sin(v*256*np.pi)
cloth = np.dstack((.34+weave*.018,.055+weave*.004,.10+weave*.006))
texture_material("curtain", cloth, weave*.025, (.35,.35), .93)
color("shelf", (.79,.81,.77,1), .25)
color("stage_line", (.22,.15,.085,1), .72)
color("wall", (.76,.77,.71,1), .82)
color("board", (.025,.052,.043,1), .84)
color("table_blue", (.027,.22,.68,1), .32)
color("table_yellow", (1.0,.63,.015,1), .32)
color("table_white", (.88,.89,.83,1), .32)
color("podium", (.48,.31,.17,1), .48)
color("metal", (.57,.62,.64,1), .24)

MAT["white_frame"] = make_material("MAT_Frame_White", (.76,.79,.77,1), .32, .12)
MAT["shell_white"] = make_material("MAT_Chair_WhiteShell", (.86,.87,.80,1), .36)
MAT["seat_orange"] = make_material("MAT_Seat_Orange", (.82,.19,.065,1), .66)
MAT["seat_blue"] = make_material("MAT_Seat_Denim", (.06,.18,.26,1), .70)
MAT["rubber"] = make_material("MAT_Caster_Rubber", (.045,.05,.048,1), .78)
MAT["panel_wood"] = make_material("MAT_Acoustic_WarmOak", (.43,.30,.19,1), .71)
MAT["slat"] = make_material("MAT_Acoustic_Slat", (.60,.46,.29,1), .61)
MAT["groove"] = make_material("MAT_Panel_Joint", (.19,.16,.125,1), .91)
MAT["line_yellow"] = make_material("MAT_FloorLine_Yellow", (.88,.65,.025,1), .52)
MAT["line_lime"] = make_material("MAT_FloorLine_Lime", (.48,.64,.13,1), .52)
MAT["book_ivory"] = make_material("MAT_Book_Paper", (.69,.67,.57,1), .88)
MAT["gold"] = make_material("MAT_Frame_Brass", (.49,.32,.11,1), .31, .72)


def box(name, size, pos, mat, bevel=0, col=None, angle=0):
    return add_box(name, size, pos, MAT[mat], col or DETAIL, rotation_z=angle, bevel=bevel)


def rod(name, a, b, radius, mat="white_frame"):
    # Geometry is authored pre-mirror and rotated in final world coordinates.
    aa, bb = Vector(mirror_location(a)), Vector(mirror_location(b))
    obj = add_cylinder(name, radius, (bb-aa).length, tuple((Vector(a)+Vector(b))*.5), MAT[mat], DETAIL, vertices=10)
    obj.rotation_euler = (bb-aa).to_track_quat("Z", "Y").to_euler()
    for poly in obj.data.polygons:
        poly.use_smooth = len(poly.vertices) == 4
    return obj


def delete_where(test):
    for obj in list(bpy.context.scene.objects):
        if test(obj):
            bpy.data.objects.remove(obj, do_unlink=True)


# Replace the old generic pedestals and block chairs with photo-guided frames.
delete_where(lambda obj: obj.name.startswith("TableCluster_") and
             ("_Chair_" in obj.name or "_Pedestal" in obj.name or "_Foot" in obj.name))
for obj in list(bpy.context.scene.objects):
    if "_TrapezoidTable_" in obj.name:
        # Thin rounded laminate top, preserving the exact six-piece hexagon.
        for vert in obj.data.vertices:
            if vert.co.z < P["TABLE_HEIGHT"] - .01:
                vert.co.z = P["TABLE_HEIGHT"] - .032
        bevel = obj.modifiers.new("Laminate_RoundedEdge", "BEVEL")
        bevel.width, bevel.segments = .008, 3


def chair(name, sx, sy, angle, index, seat_height=.46, tall=False):
    # Local radial x points away from the desk; local y runs across the seat.
    c, s = math.cos(angle), math.sin(angle)
    def at(x,y,z): return (sx+c*x-s*y, sy+s*x+c*y, z)
    box(name+"_ShellSeat", (.43,.45,.032), at(0,0,seat_height), "shell_white", .045, angle=angle)
    box(name+"_Cushion", (.40,.41,.035), at(-.01,0,seat_height+.026),
        "seat_orange" if index%2 else "seat_blue", .045, angle=angle)
    # Gently bowed white backrest with softened perimeter.
    points = []
    for j in range(9):
        yy = -.235 + j*.47/8
        xx = .21 + .06*(1-(yy/.235)**2)
        points.append((xx,yy))
    verts = [mirror_location(at(x,y,z)) for z in (seat_height+.10,seat_height+.49) for x,y in points]
    faces = [(j,j+1,10+j,9+j) for j in range(8)]
    mesh = bpy.data.meshes.new(name+"_BackMesh")
    mesh.from_pydata(verts,[],faces); mesh.update()
    obj = bpy.data.objects.new(name+"_CurvedWhiteBack",mesh); DETAIL.objects.link(obj)
    obj.data.materials.append(MAT["shell_white"])
    solid = obj.modifiers.new("MouldedShell", "SOLIDIFY"); solid.thickness=.025
    bevel = obj.modifiers.new("SoftPerimeter", "BEVEL"); bevel.width=.025; bevel.segments=4
    for yy in (-.18,.18):
        rod(name+"_BackSupport",at(.16,yy,seat_height-.035),at(.255,yy,seat_height+.29),.015)
    if tall:
        rod(name+"_Lift", at(0,0,.18),at(0,0,seat_height-.02), .035, "metal")
    for k,(xx,yy) in enumerate(((-.20,-.22),(-.20,.22),(.22,-.22),(.22,.22))):
        top = at(xx*.65,yy*.72,seat_height-.015)
        foot = at(xx*1.3,yy*1.3,.065)
        rod(name+f"_Leg{k}", top,foot,.018)
        rod(name+f"_Wheel{k}", (foot[0]-.026,foot[1],.046), (foot[0]+.026,foot[1],.046), .042,"rubber")
        rod(name+f"_Hub{k}", (foot[0]-.028,foot[1],.046), (foot[0]+.028,foot[1],.046), .024,"shell_white")
    rod(name+"_UnderSeat",at(-.16,0,seat_height-.08),at(.18,0,seat_height-.08),.018)


cluster = 0
for cy in P["TABLE_CLUSTER_Y"]:
    for cx in P["TABLE_CLUSTER_X"]:
        cluster += 1
        outer, inner = regular_hex_vertices(cx,cy,P["TABLE_OUTER_HEX_RADIUS"]), regular_hex_vertices(cx,cy,P["TABLE_INNER_HEX_RADIUS"])
        for k in range(6):
            a=math.radians(60+k*60)
            label=f"PhotoTable_{cluster}_{k}"
            # Two slim end legs and a white modesty panel follow each trapezoid.
            ends=[]
            for j in (k,(k+1)%6):
                x=.65*outer[j][0]+.35*inner[j][0]; y=.65*outer[j][1]+.35*inner[j][1]
                x=cx+(x-cx)*.94; y=cy+(y-cy)*.94
                ends.append((x,y,.38))
                box(label+f"_Leg{j}",(.045,.045,.64),(x,y,.38),"white_frame",.006)
                rod(label+f"_Caster{j}",(x-.025,y,.05),(x+.025,y,.05),.04,"rubber")
            add_wall_between(label+"_ModestyPanel",ends[0][:2],ends[1][:2],.02,.39,MAT["shelf"],DETAIL,z0=.27)
            sx=cx+P["CHAIR_RADIUS"]*math.cos(a); sy=cy+P["CHAIR_RADIUS"]*math.sin(a)
            # Tiny, deterministic chair offsets avoid an unrealistically rigid array.
            sx+=P["CHAIR_POSITION_JITTER"]*math.sin(cluster*3+k)
            sy+=P["CHAIR_POSITION_JITTER"]*math.cos(cluster+k*2)
            chair(f"PhotoChair_{cluster}_{k}",sx,sy,a+.04*math.sin(k+cluster),cluster+k)

# Continuous lower tread: 21 cm + 21 cm, matching the two levels in the photos.
step_y = (P["STAGE_STEP_FRONT_Y"]+P["STAGE_FRONT_Y"])/2
box("PhotoStage_LowerTread",(P["STAGE_STEP_WIDTH"],P["STAGE_FRONT_Y"]-P["STAGE_STEP_FRONT_Y"],P["STAGE_STEP_HEIGHT"]),
    (0,step_y,P["STAGE_STEP_HEIGHT"]/2),"stage_wood",.006)
delete_where(lambda o: o.name.startswith("Stage_PlankLine_"))
# Fine joints across boards, staggered ends; no thick black marker-like lines.
for k in range(1,18):
    y=P["STAGE_FRONT_Y"]+k*.19
    half=long_half+(short_half-long_half)*(y-stage_y0)/(L-stage_y0)
    add_curve_line(f"PhotoStage_Joint{k}",[(-half+.10,y,.421),(half-.10,y,.421)],MAT["stage_line"],DETAIL,.0012)
    for x in np.arange(-half+1+(k%3)*.32,half-.2,1.15):
        add_curve_line(f"PhotoStage_End{k}_{x:.2f}",[(float(x),y-.18,.421),(float(x),y,.421)],MAT["stage_line"],DETAIL,.001)
for k in range(1,4):
    y=P["STAGE_STEP_FRONT_Y"]+k*.19
    add_curve_line(f"PhotoStep_Joint{k}",[(-5.68,y,.211),(5.68,y,.211)],MAT["stage_line"],DETAIL,.0012)
for x in (-4,0,4):
    box("PhotoStage_PowerSocket",(.14,.012,.07),(x,13.793,.115),"shelf",.007)

# Warm panel finish on the rear stage envelope and teaching wall.
for obj in (left_slant,right_slant,left_front_prop_wall,right_front_prop_wall):
    obj.data.materials.clear(); obj.data.materials.append(MAT["panel_wood"])

# Real openings in both sloping partitions, with flush matching wooden leaves.
# The editable leaves stay in Blender; the runtime exporter excludes them and
# exports their hinge geometry to C++ so interaction cannot drift from the holes.
door_specs = []
for side, old_wall in ((-1,left_slant),(1,right_slant)):
    bpy.data.objects.remove(old_wall, do_unlink=True)
    start = Vector((side*long_half, stage_y0))
    finish = Vector((side*short_half, L))
    tangent = (finish-start).normalized()
    normal = Vector((-side*tangent.y,side*tangent.x))  # toward the stage
    length = (finish-start).length
    offset, width = P["PROP_DOOR_START"], P["PROP_DOOR_WIDTH"]
    height, gap = P["PROP_DOOR_HEIGHT"], P["PROP_DOOR_GAP"]
    base, thickness = P["STAGE_HEIGHT"], P["PROP_DOOR_THICKNESS"]
    assert offset > .10 and offset+width < length-.10 and base+height < H
    a, b = start+tangent*offset, start+tangent*(offset+width)
    for label,p0,p1,z0,z1 in (("Front",start,a,0,H),("Rear",b,finish,0,H),
                             ("Header",a,b,base+height,H),("Sill",a,b,0,base)):
        add_wall_between(f"PropDoor{side}_{label}",p0,p1,T,z1-z0,MAT["panel_wood"],COL["stage"],z0=z0)
    pivot = a+normal*(T/2-thickness/2)+tangent*gap
    leaf_end = pivot+tangent*(width-2*gap)
    leaf = add_wall_between(f"PropDoor{side}_FlushLeaf",pivot,leaf_end,thickness,height-2*gap,
                           MAT["panel_wood"],COL["stage"],z0=base+gap)
    leaf["runtime_dynamic_prop_door"] = True
    # A narrow shadow gap is the only front detail, as visible in the reference.
    step_mid = (a+b)*.5-normal*(T/2+.23)
    box(f"PropDoor{side}_InteriorStep",(width,.46,.21),(*step_mid,.105),
        "stage_wood",.003,angle=math.atan2(tangent.y,tangent.x))
    world_pivot = mirror_location((pivot.x,pivot.y,base+gap))
    ue_tangent = Vector((MODEL_X_SIGN*tangent.x,-tangent.y))
    ue_normal = Vector((MODEL_X_SIGN*normal.x,-normal.y))
    import json
    door_specs.append(dict(hinge=[world_pivot[0]*100,-world_pivot[1]*100,world_pivot[2]*100],
        yaw=math.degrees(math.atan2(ue_tangent.y,ue_tangent.x)),
        swing=side*MODEL_X_SIGN*95, width=(width-2*gap)*100,
        height=(height-2*gap)*100, thickness=thickness*100,
        normal=[ue_normal.x,ue_normal.y,0]))
scene["runtime_prop_doors"] = json.dumps(door_specs)
box("PhotoStage_BackCladding",(P["STAGE_SHORT_WIDTH"],.025,H-.42),(0,L-.014,(H+.42)/2),"panel_wood")
for x in np.arange(-short_half+.74,short_half,.75):
    box("PhotoStage_PanelJoint",(.006,.029,H-.43),(float(x),L-.030,(H+.42)/2),"groove")
for side in (-1,1):
    for i in range(27):
        y=14.0+i*.075
        # Front sides have visible vertical battens; length is a working estimate.
        if y<14.55:
            box(f"PhotoStage_Slat{side}_{i}",(.034,.038,3.0),(side*5.74,y,1.72),"slat",.003)

# Curtain shape: smooth sinusoidal pleats plus a gentle sag at the hem.
delete_where(lambda o: o.name.startswith("Curtain_") and o.type=="MESH")
for side in (-1,1):
    verts=[]; faces=[]; nx=128; nz=8
    for iz in range(nz+1):
        z=.21+3.0*iz/nz
        for ix in range(nx+1):
            t=ix/nx; x=side*(5.70-1.1*t)
            y=14.36+.065*math.cos(t*16*2*math.pi)
            zz=z+.008*math.sin(t*2*math.pi)*(1-iz/nz)
            verts.append(mirror_location((x,y,zz)))
    for iz in range(nz):
        for ix in range(nx):
            a=iz*(nx+1)+ix; faces.append((a,a+1,a+nx+2,a+nx+1))
    mesh=bpy.data.meshes.new(f"Curtain_Pleats{side}"); mesh.from_pydata(verts,[],faces); mesh.update()
    obj=bpy.data.objects.new("Curtain_Photo_Left" if side<0 else "Curtain_Photo_Right",mesh)
    COL["stage"].objects.link(obj); obj.data.materials.append(MAT["curtain"])
    for p in mesh.polygons:p.use_smooth=True
    solid=obj.modifiers.new("FabricThickness","SOLIDIFY");solid.thickness=.006
    obj["runtime_dynamic_curtain"]=True

# Replace waist-high generic shelves by tall combination cabinets.
delete_where(lambda o:o.name.startswith("Bookshelf_"))
for side, centers in ((-1,[2.8+i*1.2 for i in range(8)]),(1,[3.7+i*1.2 for i in range(6)]+[13.05])):
    for idx,y in enumerate(centers):
        x=side*5.72; front=side*5.49; h=P["CABINET_PHOTO_HEIGHT"]
        name=f"PhotoCabinet_{side}_{idx}"
        box(name+"_Back",(.035,1.19,h),(side*5.95,y,h/2),"shelf")
        for off in (-.585,.585):box(name+"_Upright",(.47,.03,h),(x,y+off,h/2),"shelf")
        for z in (.045,.92,1.48,2.02,2.58,3.17):box(name+"_Shelf",(.47,1.18,.027),(x,y,z),"shelf")
        box(name+"_TopDoor",(.038,1.18,.57),(front,y,2.875),"shelf",.004)
        for k in (-1,1):
            box(name+"_BaseDoor",(.035,.578,.85),(front,y+k*.296,.465),"shelf",.004)
            box(name+"_Handle",(.028,.018,.145),(front-side*.033,y+k*.055,.64),"metal",.004)
        if idx%3!=1:
            box(name+"_UpperSlidingDoor",(.03,.57,1.61),(front-side*.008,y+.27,1.75),"shelf",.004)
        for row,z in enumerate((.94,1.50,2.04)):
            for j in range(5):
                yy=y-.49+j*.075; bh=.23+.025*((j+row+idx)%4)
                mat=["book_ivory","book_blue","book_ivory","book_red","book_ivory"][(j+idx)%5]
                box(name+f"_Book{row}_{j}",(.23,.046,bh),(side*5.65,yy,z+bh/2),mat,.003)

# Two standing desks near stage: locations estimated, existence photo-confirmed.
for idx,y in enumerate((12.65,1.05)):
    x=-5.65
    box(f"PhotoCounter{idx}_Top",(.59,1.75,.045),(x,y,1.04),"shelf",.008)
    box(f"PhotoCounter{idx}_Back",(.025,1.75,1.0),(-5.945,y,.51),"panel_wood")
    for yy in (y-.83,y+.83):box(f"PhotoCounter{idx}_Leg",(.04,.04,1.02),(-5.38,yy,.51),"white_frame")
    box(f"PhotoCounter{idx}_FootRail",(.035,1.69,.035),(-5.38,y,.20),"white_frame")
    for j in range(5):
        yy=y-.31+j*.14
        box(f"PhotoCounter{idx}_Outlet",(.02,.11,.085),(-5.919,yy,.32),"shell_white",.006)
        for zz in (.31,.335):box("OutletSocket",(.022,.027,.009),(-5.905,yy,zz),"rubber")
    for j in (-1,1):chair(f"PhotoCounter{idx}_Stool{j}",-5.05,y+j*.42,math.pi,idx+j,.72,True)
    # Simple framed abstract relief is an original placeholder, not copied art.
    box(f"PhotoFrame{idx}_Backing",(.034,.64,.78),(-5.94,y,2.10),"book_ivory")
    box(f"PhotoFrame{idx}_Field",(.037,.51,.65),(-5.915,y,2.10),"seat_blue")
    for yy in (y-.32,y+.32):box("PhotoFrame_Side",(.046,.035,.81),(-5.901,yy,2.10),"gold",.005)
    for zz in (1.70,2.50):box("PhotoFrame_Rail",(.046,.66,.035),(-5.901,y,zz),"gold",.005)
    for j in range(4):box("PhotoFrame_AbstractBand",(.04,.43-.05*j,.05),(-5.888,y+.01*j,1.90+j*.13),"book_ivory",.01)

# Grey charging cart, vent slots and cable loops (photo evidence, count assumed).
box("PhotoChargingCart",(.48,.62,.88),(-5.31,10.9,.45),"metal",.012)
box("PhotoChargingCart_Opening",(.012,.52,.62),(-5.06,10.9,.48),"rubber",.012)
for j in range(7):
    yy=10.67+j*.071
    box("ChargingCart_Divider",(.015,.009,.60),(-5.045,yy,.48),"white_frame")
    pts=[(-5.025,yy+.019*math.sin(t*6.28),.65-t*.39) for t in np.linspace(0,1,20)]
    add_curve_line("ChargingCart_Cable",pts,MAT["book_ivory"],DETAIL,.002)

# Floor inlay arcs use a thin surface mesh, following the photo's broad stripes.
for k,(cy,rx,ry,mat) in enumerate(((6.7,5.3,3.9,"line_yellow"),(10.7,4.95,4.25,"line_lime"))):
    verts=[]
    for radius in (0, .13):
        for j in range(161):
            a=2*math.pi*j/160
            verts.append(mirror_location(((rx-radius)*math.cos(a),cy+(ry-radius)*math.sin(a),.003)))
    faces=[(j,j+1,j+162,j+161) for j in range(160)]
    mesh=bpy.data.meshes.new("FloorArcMesh");mesh.from_pydata(verts,[],faces);mesh.update()
    obj=bpy.data.objects.new(f"PhotoFloorArc{k}",mesh);DETAIL.objects.link(obj);mesh.materials.append(MAT[mat])

# Confirmed sliding-board mechanism, shared with the runtime exporter.
runpy.run_path(str(OUT_DIR / "Tools" / "Blender" / "teaching_wall.py"), init_globals=globals())

# Narrow ceiling panel joints and cassette AC. Hidden only in cutaway Blender
# previews, included in runtime export; no natural light is added to the room.
for y in np.arange(.60,L,.60):box("PhotoCeiling_Seam",(W,.007,.003),(0,float(y),H-.001),"groove",col=OVERHEAD)
for x in np.arange(-4.8,6,1.2):box("PhotoCeiling_LongSeam",(.006,L,.003),(float(x),L/2,H-.001),"groove",col=OVERHEAD)
for idx,y in enumerate((3.6,9.0,16.55)):
    box(f"PhotoAC{idx}_Bezel",(.96,.96,.045),(0,y,H-.033),"shelf",.04,col=OVERHEAD)
    box(f"PhotoAC{idx}_Intake",(.67,.67,.017),(0,y,H-.061),"groove",.025,col=OVERHEAD)
    for j in range(15):box(f"PhotoAC{idx}_Mesh",(.63,.009,.006),(0,y-.30+j*.043,H-.073),"shelf",col=OVERHEAD)
    for sign in (-1,1):
        box(f"PhotoAC{idx}_OutletX",(.047,.79,.017),(sign*.405,y,H-.061),"rubber",.006,col=OVERHEAD)
        box(f"PhotoAC{idx}_OutletY",(.79,.047,.017),(0,y+sign*.405,H-.061),"rubber",.006,col=OVERHEAD)
for y in (6.8,12.8):add_cylinder("PhotoCeiling_SmokeDetector",.045,.018,(1.5,y,H-.025),MAT["shelf"],OVERHEAD,24)
for obj in OVERHEAD.objects:obj.hide_render=True

# UVs use world-meter projections so both the editable Blender file and joined
# runtime mesh retain the same wood-grain / vinyl / woven-cloth scale.
bpy.context.view_layer.update()
for obj in bpy.context.scene.objects:
    if obj.type!="MESH" or not obj.data.materials:continue
    repeat=obj.data.materials[0].get("uv_repeat_m")
    if repeat is None:continue
    uv=obj.data.uv_layers.new(name="PhotoUV")
    normal_matrix=obj.matrix_world.to_3x3()
    for poly in obj.data.polygons:
        normal=normal_matrix@poly.normal
        axes=(0,1) if abs(normal.z)>.6 else ((0,2) if abs(normal.y)>.6 else (1,2))
        for li in poly.loop_indices:
            co=obj.matrix_world@obj.data.vertices[obj.data.loops[li].vertex_index].co
            uv.data[li].uv=(co[axes[0]]/repeat[0],co[axes[1]]/repeat[1])

scene["photo_revision"]="2026-09-05: four user reference photos; dimensions provisional"
print("PHOTO_DETAILS_COMPLETE objects=", len(bpy.context.scene.objects))
