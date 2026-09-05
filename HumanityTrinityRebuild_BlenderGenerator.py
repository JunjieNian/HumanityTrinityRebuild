"""
复旦附中笃志楼 B1 “三一人文空间”参数化粗模

运行方式（Windows / Blender 4.5 LTS）：
    blender.exe --background --python HumanityTrinityRebuild_BlenderGenerator.py

本脚本会在自身所在目录生成：
    HumanityTrinityRebuild.blend
    HumanityTrinityRebuild.glb
    HumanityTrinityRebuild_TopView.png
    HumanityTrinityRebuild_PerspectiveToStage.png
    HumanityTrinityRebuild_PerspectiveToTeaching.png

坐标约定：
    X：教室左右，左负右正
    Y：教室纵向，前方教学墙为 0，后墙为 ROOM_LENGTH
    Z：高度，地面为 0

已确认的结构约束与暂定尺寸的区分，见同目录《三一人文空间_建模说明.md》。
"""

from pathlib import Path
import json
import math
import runpy

import bpy
from mathutils import Vector


# ============================================================================
# 1. 关键参数：后续迭代优先只改这里
# ============================================================================

P = {
    # 已按现场反馈沿教室纵向中轴整体镜像：三扇门位于左侧墙。
    "MIRROR_ACROSS_LONGITUDINAL_AXIS": True,

    # 房间外壳（暂定）
    "ROOM_WIDTH": 12.0,
    "ROOM_LENGTH": 18.0,
    "ROOM_HEIGHT": 3.4,
    "WALL_THICKNESS": 0.18,
    "FLOOR_THICKNESS": 0.12,

    # 舞台（结构已确认，尺寸暂定）
    "STAGE_FRONT_Y": 14.55,
    "STAGE_HEIGHT": 0.42,
    "STAGE_LONG_WIDTH": 9.30,
    "STAGE_SHORT_WIDTH": 5.90,

    # 幕布（位置已确认；第一版拉开以展示舞台）
    "CURTAIN_Y": 14.36,
    "CURTAIN_RAIL_HEIGHT": 3.24,
    "CURTAIN_OPENING_HALF_WIDTH": 4.60,
    # 照片确认的两级台阶与室内细节，尺度仍为估计。
    "PHOTO_DETAILS": True,
    "STAGE_STEP_FRONT_Y": 13.80,
    "STAGE_STEP_HEIGHT": 0.21,
    "STAGE_STEP_WIDTH": 11.40,
    "CABINET_PHOTO_HEIGHT": 3.18,

    # 左侧三门（顺序和作用已确认，具体位置/宽度暂定）
    "DOOR_WIDTH": 1.15,
    "DOOR_HEIGHT": 2.15,
    "FRONT_DOOR_Y": 2.40,
    "MIDDLE_DOOR_Y": 12.00,
    "REAR_DOOR_Y": 16.40,

    # 书架/资源柜（位于两侧已确认，具体尺寸和段数暂定）
    "SHELF_DEPTH": 0.44,
    "SHELF_HEIGHT": 2.05,
    "SHELF_MODULE_WIDTH": 1.12,

    # 小组桌：每组由 6 张等形梯形桌拼成中空正六边形（形状已确认）
    "TABLE_CLUSTER_X": [-3.25, 0.0, 3.25],
    "TABLE_CLUSTER_Y": [4.65, 8.10, 11.55],
    "TABLE_HEIGHT": 0.76,
    # 内外轮廓均为同心、同朝向的正六边形；两者之间的每一侧就是一张等腰梯形桌。
    "TABLE_OUTER_HEX_RADIUS": 1.08,
    "TABLE_INNER_HEX_RADIUS": 0.42,
    "CHAIR_RADIUS": 1.43,
}


OUT_DIR = Path(__file__).resolve().parent
BLEND_PATH = OUT_DIR / "HumanityTrinityRebuild.blend"
GLB_PATH = OUT_DIR / "HumanityTrinityRebuild.glb"
TOP_RENDER_PATH = OUT_DIR / "HumanityTrinityRebuild_TopView.png"
STAGE_RENDER_PATH = OUT_DIR / "HumanityTrinityRebuild_PerspectiveToStage.png"
TEACHING_RENDER_PATH = OUT_DIR / "HumanityTrinityRebuild_PerspectiveToTeaching.png"


# ============================================================================
# 2. 基础工具
# ============================================================================

MODEL_X_SIGN = -1.0 if P["MIRROR_ACROSS_LONGITUDINAL_AXIS"] else 1.0


def mirror_x(x):
    return MODEL_X_SIGN * x


def mirror_location(location):
    return (mirror_x(location[0]), location[1], location[2])


def mirror_side_text(value):
    """Keep object names and human-readable left/right labels correct after mirroring."""
    if MODEL_X_SIGN > 0.0:
        return value
    return (
        value
        .replace("Left", "__HTR_LEFT__")
        .replace("Right", "Left")
        .replace("__HTR_LEFT__", "Right")
        .replace("left", "__htr_left__")
        .replace("right", "left")
        .replace("__htr_left__", "right")
        .replace("左", "__三一左__")
        .replace("右", "左")
        .replace("__三一左__", "右")
    )

def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (
        bpy.data.meshes,
        bpy.data.curves,
        bpy.data.materials,
        bpy.data.cameras,
        bpy.data.lights,
    ):
        # 仅清理孤立数据，避免 Blender 默认场景残留。
        for block in list(datablocks):
            if block.users == 0:
                datablocks.remove(block)


def make_collection(name):
    collection = bpy.data.collections.new(mirror_side_text(name))
    bpy.context.scene.collection.children.link(collection)
    return collection


def move_to_collection(obj, collection):
    for current in list(obj.users_collection):
        current.objects.unlink(obj)
    collection.objects.link(obj)


def make_material(name, color, roughness=0.55, metallic=0.0, emission=None):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic
    if emission is not None:
        bsdf.inputs["Emission Color"].default_value = emission[0]
        bsdf.inputs["Emission Strength"].default_value = emission[1]
    return mat


def add_box(name, dimensions, location, material, collection, rotation_z=0.0, bevel=0.0):
    # Direct mesh creation keeps thousands-of-parts rebuilds fast.
    dx, dy, dz = (d / 2 for d in dimensions)
    verts = [(-dx,-dy,-dz),(dx,-dy,-dz),(dx,dy,-dz),(-dx,dy,-dz),
             (-dx,-dy,dz),(dx,-dy,dz),(dx,dy,dz),(-dx,dy,dz)]
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(verts, [], [(3,2,1,0),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)])
    mesh.update()
    obj = bpy.data.objects.new(mirror_side_text(name), mesh)
    obj.location = mirror_location(location)
    obj.rotation_euler.z = MODEL_X_SIGN * rotation_z
    collection.objects.link(obj)
    if material:
        obj.data.materials.append(material)
    if bevel > 0.0:
        modifier = obj.modifiers.new(name="Soft_Edges", type="BEVEL")
        modifier.width = bevel
        modifier.segments = 3
    move_to_collection(obj, collection)
    return obj


def add_cylinder(name, radius, depth, location, material, collection, vertices=32):
    pts = [(radius*math.cos(i*2*math.pi/vertices), radius*math.sin(i*2*math.pi/vertices), z)
           for z in (-depth/2, depth/2) for i in range(vertices)]
    faces = [tuple(reversed(range(vertices))), tuple(range(vertices,2*vertices))]
    faces += [(i,(i+1)%vertices,(i+1)%vertices+vertices,i+vertices) for i in range(vertices)]
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(pts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(mirror_side_text(name), mesh)
    obj.location = mirror_location(location)
    collection.objects.link(obj)
    if material:
        obj.data.materials.append(material)
    move_to_collection(obj, collection)
    return obj


def add_prism(name, xy_points, z0, z1, material, collection):
    xy_points = [(mirror_x(x), y) for x, y in xy_points]
    if MODEL_X_SIGN < 0.0:
        xy_points.reverse()
    count = len(xy_points)
    vertices = [(x, y, z0) for x, y in xy_points] + [(x, y, z1) for x, y in xy_points]
    faces = []
    faces.append(tuple(range(count - 1, -1, -1)))
    faces.append(tuple(range(count, count * 2)))
    for i in range(count):
        j = (i + 1) % count
        faces.append((i, j, count + j, count + i))

    name = mirror_side_text(name)
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    if material:
        obj.data.materials.append(material)
    return obj


def add_flat_polygon(name, xy_points, z, material, collection):
    xy_points = [(mirror_x(x), y) for x, y in xy_points]
    if MODEL_X_SIGN < 0.0:
        xy_points.reverse()
    name = mirror_side_text(name)
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata([(x, y, z) for x, y in xy_points], [], [tuple(range(len(xy_points)))])
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    if material:
        obj.data.materials.append(material)
    return obj


def add_wall_between(name, start_xy, end_xy, thickness, height, material, collection, z0=0.0):
    x0, y0 = start_xy
    x1, y1 = end_xy
    dx = x1 - x0
    dy = y1 - y0
    length = math.hypot(dx, dy)
    angle = math.atan2(dy, dx)
    return add_box(
        name=name,
        dimensions=(length, thickness, height),
        location=((x0 + x1) / 2, (y0 + y1) / 2, z0 + height / 2),
        material=material,
        collection=collection,
        rotation_z=angle,
    )


def add_curve_line(name, points, material, collection, bevel_depth=0.012, cyclic=False):
    name = mirror_side_text(name)
    points = [(mirror_x(point[0]), point[1], point[2]) for point in points]
    curve = bpy.data.curves.new(name + "_Curve", type="CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = 2
    curve.bevel_depth = bevel_depth
    curve.bevel_resolution = 2
    spline = curve.splines.new("POLY")
    spline.points.add(len(points) - 1)
    for point, co in zip(spline.points, points):
        point.co = (co[0], co[1], co[2], 1.0)
    spline.use_cyclic_u = cyclic
    obj = bpy.data.objects.new(name, curve)
    collection.objects.link(obj)
    if material:
        obj.data.materials.append(material)
    return obj


def add_text(name, body, location, size, material, collection, rotation_z=0.0, align="CENTER"):
    name = mirror_side_text(name)
    body = mirror_side_text(body)
    curve = bpy.data.curves.new(name + "_Curve", type="FONT")
    curve.body = body
    curve.align_x = align
    curve.align_y = "CENTER"
    curve.size = size
    curve.extrude = 0.006
    curve.bevel_depth = 0.002
    obj = bpy.data.objects.new(name, curve)
    obj.location = mirror_location(location)
    obj.rotation_euler[2] = MODEL_X_SIGN * rotation_z
    collection.objects.link(obj)
    if material:
        obj.data.materials.append(material)
    return obj


def aim_object(obj, target):
    direction = Vector(target) - obj.location
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()


def set_render(scene, filepath, width, height):
    scene.render.filepath = str(filepath)
    scene.render.resolution_x = width
    scene.render.resolution_y = height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    bpy.ops.render.render(write_still=True)


# ============================================================================
# 3. 场景初始化、集合与材质
# ============================================================================

clear_scene()

scene = bpy.context.scene
scene.name = "HumanityTrinityRebuild"
scene.unit_settings.system = "METRIC"
scene.unit_settings.length_unit = "METERS"
scene.render.engine = "BLENDER_EEVEE_NEXT"
scene.render.image_settings.color_mode = "RGBA"
scene.render.film_transparent = False
scene.render.resolution_percentage = 100
scene.render.use_file_extension = True
scene.render.image_settings.compression = 18
scene.render.engine = "BLENDER_EEVEE_NEXT"
scene.world.color = (0.035, 0.045, 0.060)

if hasattr(scene, "view_settings"):
    scene.view_settings.look = "AgX - Medium High Contrast"

COL = {
    "shell": make_collection("00_Shell_空间外壳"),
    "teaching": make_collection("01_Teaching_前方教学端"),
    "furniture": make_collection("02_Furniture_中央桌椅"),
    "storage": make_collection("03_Storage_两侧书架柜体"),
    "stage": make_collection("04_Stage_舞台幕布道具间"),
    "doors": make_collection("05_Doors_右侧三门"),
    "lights": make_collection("06_Lighting_人工照明"),
    "annotations": make_collection("07_Annotations_暂定尺寸标注"),
    "cameras": make_collection("99_Cameras_相机"),
}

MAT = {
    "floor": make_material("MAT_Floor_WarmGrey", (0.42, 0.38, 0.32, 1), roughness=0.75),
    "wall": make_material("MAT_Wall_OffWhite", (0.84, 0.84, 0.80, 1), roughness=0.82),
    "wall_trim": make_material("MAT_Trim_Dark", (0.08, 0.10, 0.12, 1), roughness=0.52),
    "stage_wood": make_material("MAT_Stage_Wood", (0.37, 0.17, 0.075, 1), roughness=0.62),
    "stage_line": make_material("MAT_Stage_PlankLine", (0.12, 0.045, 0.018, 1), roughness=0.75),
    "curtain": make_material("MAT_Curtain_Burgundy", (0.31, 0.012, 0.025, 1), roughness=0.92),
    "shelf": make_material("MAT_Shelf_White", (0.87, 0.89, 0.89, 1), roughness=0.72),
    "book_blue": make_material("MAT_Book_Blue", (0.055, 0.22, 0.40, 1), roughness=0.70),
    "book_red": make_material("MAT_Book_Red", (0.52, 0.08, 0.055, 1), roughness=0.70),
    "book_yellow": make_material("MAT_Book_Yellow", (0.82, 0.48, 0.045, 1), roughness=0.70),
    "board": make_material("MAT_Chalkboard_Green", (0.025, 0.145, 0.105, 1), roughness=0.82),
    "screen": make_material(
        "MAT_Display_Screen",
        (0.018, 0.035, 0.055, 1),
        roughness=0.28,
        emission=((0.06, 0.15, 0.23, 1), 0.7),
    ),
    "podium": make_material("MAT_Podium_Wood", (0.40, 0.20, 0.075, 1), roughness=0.58),
    "table_white": make_material("MAT_Table_White", (0.89, 0.90, 0.88, 1), roughness=0.68),
    "table_yellow": make_material("MAT_Table_Yellow", (0.91, 0.57, 0.065, 1), roughness=0.67),
    "table_blue": make_material("MAT_Table_Blue", (0.075, 0.35, 0.58, 1), roughness=0.67),
    "chair": make_material("MAT_Chair_Charcoal", (0.075, 0.09, 0.115, 1), roughness=0.68),
    "metal": make_material("MAT_Metal", (0.14, 0.16, 0.18, 1), roughness=0.32, metallic=0.62),
    "door": make_material("MAT_Door_Wood", (0.27, 0.12, 0.045, 1), roughness=0.58),
    "prop_floor": make_material("MAT_PropRoom_Floor", (0.10, 0.14, 0.17, 1), roughness=0.80),
    "annotation": make_material(
        "MAT_Annotation_Blue",
        (0.035, 0.18, 0.42, 1),
        roughness=0.45,
        emission=((0.035, 0.18, 0.42, 1), 0.8),
    ),
    "light_panel": make_material(
        "MAT_Light_Panel",
        (0.95, 0.96, 1.0, 1),
        roughness=0.25,
        emission=((0.95, 0.96, 1.0, 1), 5.0),
    ),
}

W = P["ROOM_WIDTH"]
L = P["ROOM_LENGTH"]
H = P["ROOM_HEIGHT"]
T = P["WALL_THICKNESS"]


# ============================================================================
# 4. 房间外壳：无窗矩形地下空间
# ============================================================================

floor = add_box(
    "Floor_地面_暂定12x18m",
    (W, L, P["FLOOR_THICKNESS"]),
    (0.0, L / 2, -P["FLOOR_THICKNESS"] / 2),
    MAT["floor"],
    COL["shell"],
)
floor["status"] = "暂定尺寸；无窗矩形地下教室为已确认结构"

front_wall = add_box(
    "Wall_Front_前方教学墙",
    (W + 2 * T, T, H),
    (0.0, -T / 2, H / 2),
    MAT["wall"],
    COL["shell"],
)
back_wall = add_box(
    "Wall_Back_后墙_舞台短边贴墙",
    (W + 2 * T, T, H),
    (0.0, L + T / 2, H / 2),
    MAT["wall"],
    COL["shell"],
)
left_wall = add_box(
    "Wall_Left_左侧无窗实体墙",
    (T, L, H),
    (-W / 2 - T / 2, L / 2, H / 2),
    MAT["wall"],
    COL["shell"],
)


def build_right_wall_with_three_openings():
    centers = [P["FRONT_DOOR_Y"], P["MIDDLE_DOOR_Y"], P["REAR_DOOR_Y"]]
    half = P["DOOR_WIDTH"] / 2
    openings = [(c - half, c + half) for c in centers]
    intervals = []
    cursor = 0.0
    for lo, hi in openings:
        if lo > cursor:
            intervals.append((cursor, lo))
        cursor = hi
    if cursor < L:
        intervals.append((cursor, L))

    wall_parts = []
    for index, (lo, hi) in enumerate(intervals, start=1):
        obj = add_box(
            f"Wall_Right_Segment_{index:02d}",
            (T, hi - lo, H),
            (W / 2 + T / 2, (lo + hi) / 2, H / 2),
            MAT["wall"],
            COL["shell"],
        )
        wall_parts.append(obj)

    header_height = H - P["DOOR_HEIGHT"]
    for index, center in enumerate(centers, start=1):
        obj = add_box(
            f"Wall_Right_DoorHeader_{index:02d}",
            (T, P["DOOR_WIDTH"], header_height),
            (W / 2 + T / 2, center, P["DOOR_HEIGHT"] + header_height / 2),
            MAT["wall"],
            COL["shell"],
        )
        wall_parts.append(obj)
    return wall_parts


right_wall_parts = build_right_wall_with_three_openings()

# 可选顶板：证明“地下、无窗、完全人工照明”，默认不参与视图和渲染。
ceiling = add_box(
    "Ceiling_顶板_默认隐藏便于查看",
    (W, L, 0.12),
    (0.0, L / 2, H + 0.06),
    MAT["wall"],
    COL["shell"],
)
ceiling.hide_render = True
ceiling.hide_set(True)
ceiling["note"] = "检查完整封闭空间时可在 Outliner 中取消隐藏"


# ============================================================================
# 5. 前方教学墙、大屏、书写板与讲台
# ============================================================================

board_z = 2.13
add_box(
    "Chalkboard_Left_左书写板",
    (2.85, 0.065, 1.38),
    (-3.48, 0.12, board_z),
    MAT["board"],
    COL["teaching"],
    bevel=0.025,
)
add_box(
    "Chalkboard_Right_右书写板",
    (2.85, 0.065, 1.38),
    (3.48, 0.12, board_z),
    MAT["board"],
    COL["teaching"],
    bevel=0.025,
)
display = add_box(
    "Display_Center_中央大屏",
    (3.86, 0.10, 1.52),
    (0.0, 0.16, 2.16),
    MAT["screen"],
    COL["teaching"],
    bevel=0.035,
)
display["status"] = "大屏/智慧黑板结构有公开照片旁证；具体尺寸暂定"

add_box(
    "Display_Frame_Top",
    (3.98, 0.13, 0.055),
    (0.0, 0.16, 2.94),
    MAT["wall_trim"],
    COL["teaching"],
)
add_box(
    "Display_Frame_Bottom",
    (3.98, 0.13, 0.055),
    (0.0, 0.16, 1.38),
    MAT["wall_trim"],
    COL["teaching"],
)

podium = add_box(
    "Podium_木质讲台_暂定位置",
    (1.38, 0.78, 1.05),
    (3.58, 1.02, 0.525),
    MAT["podium"],
    COL["teaching"],
    bevel=0.045,
)
podium["status"] = "讲台存在已确认；位置和大小暂定；前门与其大致相对"
add_box(
    "Podium_Top",
    (1.48, 0.88, 0.07),
    (3.58, 1.02, 1.085),
    MAT["podium"],
    COL["teaching"],
    bevel=0.035,
)
add_box(
    "Podium_Inset",
    (0.82, 0.025, 0.52),
    (3.58, 1.425, 0.56),
    MAT["wall_trim"],
    COL["teaching"],
    bevel=0.02,
)


# ============================================================================
# 6. 右侧三扇门：前门、中门、后门
# ============================================================================

def build_door(name, center_y, label):
    door_w = P["DOOR_WIDTH"]
    door_h = P["DOOR_HEIGHT"]
    frame = 0.075
    x_frame = W / 2 + 0.005

    add_box(
        name + "_Frame_LowY",
        (T + 0.07, frame, door_h),
        (x_frame, center_y - door_w / 2, door_h / 2),
        MAT["wall_trim"],
        COL["doors"],
    )
    add_box(
        name + "_Frame_HighY",
        (T + 0.07, frame, door_h),
        (x_frame, center_y + door_w / 2, door_h / 2),
        MAT["wall_trim"],
        COL["doors"],
    )
    add_box(
        name + "_Frame_Top",
        (T + 0.07, door_w + 2 * frame, frame),
        (x_frame, center_y, door_h),
        MAT["wall_trim"],
        COL["doors"],
    )

    # 门扇以约 72 度打开，帮助俯视图辨认门洞和开启方向。
    angle = math.radians(72)
    hinge = (W / 2 - 0.015, center_y - door_w / 2 + 0.04)
    end = (
        hinge[0] - (door_w - 0.08) * math.sin(angle),
        hinge[1] + (door_w - 0.08) * math.cos(angle),
    )
    slab = add_wall_between(
        name + "_DoorLeaf_Open72deg",
        hinge,
        end,
        0.055,
        door_h - 0.08,
        MAT["door"],
        COL["doors"],
        z0=0.04,
    )
    slab["label"] = mirror_side_text(label)
    slab["status"] = "门的位置和宽度暂定；三门顺序/作用来自现场信息"
    return slab


front_door = build_door("Door_01_Front_前门", P["FRONT_DOOR_Y"], "前门：大致对讲台")
middle_door = build_door("Door_02_Middle_中门", P["MIDDLE_DOOR_Y"], "中门：约对最后一排小组桌")
rear_door = build_door("Door_03_Rear_后门", P["REAR_DOOR_Y"], "后门：直通右侧道具间")


# ============================================================================
# 7. 后部梯形木舞台、幕布与左右道具间
# ============================================================================

stage_y0 = P["STAGE_FRONT_Y"]
stage_y1 = L
long_half = P["STAGE_LONG_WIDTH"] / 2
short_half = P["STAGE_SHORT_WIDTH"] / 2
stage_polygon = [
    (-long_half, stage_y0),
    (long_half, stage_y0),
    (short_half, stage_y1),
    (-short_half, stage_y1),
]

stage = add_prism(
    "Stage_Trapezoid_梯形架高木舞台",
    stage_polygon,
    0.0,
    P["STAGE_HEIGHT"],
    MAT["stage_wood"],
    COL["stage"],
)
stage["status"] = "梯形、架高、木制、长边向中央、短边贴后墙均已确认；尺寸暂定"

# 木地板拼缝强调梯形方向。
stage_depth = stage_y1 - stage_y0
for index in range(1, 11):
    frac = index / 11
    y = stage_y0 + stage_depth * frac
    half_width = long_half + (short_half - long_half) * frac
    add_curve_line(
        f"Stage_PlankLine_{index:02d}",
        [(-half_width + 0.05, y, P["STAGE_HEIGHT"] + 0.006),
         (half_width - 0.05, y, P["STAGE_HEIGHT"] + 0.006)],
        MAT["stage_line"],
        COL["stage"],
        bevel_depth=0.008,
    )

left_prop_polygon = [
    (-W / 2, stage_y0),
    (-long_half, stage_y0),
    (-short_half, stage_y1),
    (-W / 2, stage_y1),
]
right_prop_polygon = [
    (long_half, stage_y0),
    (W / 2, stage_y0),
    (W / 2, stage_y1),
    (short_half, stage_y1),
]
add_flat_polygon(
    "PropRoom_Left_Floor_左道具间地面",
    left_prop_polygon,
    0.012,
    MAT["prop_floor"],
    COL["stage"],
)
add_flat_polygon(
    "PropRoom_Right_Floor_右道具间地面",
    right_prop_polygon,
    0.012,
    MAT["prop_floor"],
    COL["stage"],
)

left_slant = add_wall_between(
    "PropRoom_Left_SlantedWall_左道具间斜隔墙",
    (-long_half, stage_y0),
    (-short_half, stage_y1),
    T,
    H,
    MAT["wall"],
    COL["stage"],
)
right_slant = add_wall_between(
    "PropRoom_Right_SlantedWall_右道具间斜隔墙",
    (long_half, stage_y0),
    (short_half, stage_y1),
    T,
    H,
    MAT["wall"],
    COL["stage"],
)
left_front_prop_wall = add_wall_between(
    "PropRoom_Left_FrontWall_左道具间前隔墙",
    (-W / 2, stage_y0),
    (-long_half, stage_y0),
    T,
    H,
    MAT["wall"],
    COL["stage"],
)
right_front_prop_wall = add_wall_between(
    "PropRoom_Right_FrontWall_右道具间前隔墙",
    (long_half, stage_y0),
    (W / 2, stage_y0),
    T,
    H,
    MAT["wall"],
    COL["stage"],
)
for wall in (left_slant, right_slant, left_front_prop_wall, right_front_prop_wall):
    wall["status"] = "道具间由矩形外壳与梯形舞台斜边之间的角部空间形成"


def create_curtain_panel(name, x0, x1, y, z0, z1, material, collection, folds=14):
    name = mirror_side_text(name)
    xs = [mirror_x(x0 + (x1 - x0) * i / folds) for i in range(folds + 1)]
    if MODEL_X_SIGN < 0.0:
        xs.reverse()
    vertices = []
    for z in (z0, z1):
        for i, x in enumerate(xs):
            wave = 0.075 * math.sin(i * math.pi)
            # 对离散点使用交替折线，确保俯视图能读出“幕布”而不是直墙。
            wave += 0.045 if i % 2 == 0 else -0.045
            vertices.append((x, y + wave, z))
    faces = []
    row = folds + 1
    for i in range(folds):
        faces.append((i, i + 1, row + i + 1, row + i))
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    obj.data.materials.append(material)
    solidify = obj.modifiers.new("Fabric_Thickness", "SOLIDIFY")
    solidify.thickness = 0.018
    return obj


rail_half = W / 2 - 0.30
add_curve_line(
    "Curtain_Rail_幕布轨道_位于桌区与舞台之间",
    [(-rail_half, P["CURTAIN_Y"], P["CURTAIN_RAIL_HEIGHT"]),
     (rail_half, P["CURTAIN_Y"], P["CURTAIN_RAIL_HEIGHT"])],
    MAT["metal"],
    COL["stage"],
    bevel_depth=0.026,
)

opening_half = P["CURTAIN_OPENING_HALF_WIDTH"]
create_curtain_panel(
    "Curtain_Left_Open_左幕布拉开",
    -rail_half,
    -opening_half,
    P["CURTAIN_Y"],
    0.08,
    P["CURTAIN_RAIL_HEIGHT"] - 0.06,
    MAT["curtain"],
    COL["stage"],
)
create_curtain_panel(
    "Curtain_Right_Open_右幕布拉开",
    opening_half,
    rail_half,
    P["CURTAIN_Y"],
    0.08,
    P["CURTAIN_RAIL_HEIGHT"] - 0.06,
    MAT["curtain"],
    COL["stage"],
)

# 幕布位置的地面投影线仅用于解释“桌区 / 幕布 / 舞台”的关系。
curtain_guide = add_curve_line(
    "Curtain_ClosedPosition_Guide_幕布闭合位置参考线",
    [(-rail_half, P["CURTAIN_Y"], 0.025), (rail_half, P["CURTAIN_Y"], 0.025)],
    MAT["curtain"],
    COL["annotations"],
    bevel_depth=0.018,
)
curtain_guide["note"] = "幕布当前为拉开状态；此线显示闭合时横跨位置"


# ============================================================================
# 8. 两侧书架与资源柜
# ============================================================================

BOOK_MATS = [MAT["book_blue"], MAT["book_red"], MAT["book_yellow"]]


def build_bookcase_unit(name, side, center_y, index):
    depth = P["SHELF_DEPTH"]
    height = P["SHELF_HEIGHT"]
    width = P["SHELF_MODULE_WIDTH"]
    inner_x = side * (W / 2 - T - depth / 2)
    back_x = side * (W / 2 - T - 0.025)
    add_box(
        name + "_Back",
        (0.05, width, height),
        (back_x, center_y, height / 2),
        MAT["shelf"],
        COL["storage"],
    )
    for offset in (-width / 2 + 0.035, width / 2 - 0.035):
        add_box(
            name + f"_Side_{offset:+.2f}",
            (depth, 0.07, height),
            (inner_x, center_y + offset, height / 2),
            MAT["shelf"],
            COL["storage"],
        )
    for level, z in enumerate((0.05, 0.50, 0.96, 1.42, 2.00)):
        add_box(
            name + f"_Shelf_{level}",
            (depth, width, 0.055),
            (inner_x, center_y, z),
            MAT["shelf"],
            COL["storage"],
        )

    # 每个单元放少量彩色书脊，保持“粗模”而不是做成繁复资产库。
    face_x = side * (W / 2 - T - depth + 0.11)
    for level, z in enumerate((0.30, 0.76, 1.22, 1.68)):
        for book_index in range(3):
            book_y = center_y - 0.34 + book_index * 0.25
            book_h = 0.25 + 0.035 * ((index + level + book_index) % 3)
            add_box(
                name + f"_Book_{level}_{book_index}",
                (0.20, 0.115, book_h),
                (face_x, book_y, z - 0.22 + book_h / 2),
                BOOK_MATS[(index + level + book_index) % len(BOOK_MATS)],
                COL["storage"],
                bevel=0.008,
            )


left_centers = [3.05 + i * 1.18 for i in range(10)]
for index, center in enumerate(left_centers, start=1):
    build_bookcase_unit(f"Bookshelf_Left_{index:02d}", -1, center, index)

# 右侧柜体避开三扇门；末段保留一组，显示“右侧也有柜体”。
right_centers = [3.65 + i * 1.18 for i in range(6)] + [13.45]
for index, center in enumerate(right_centers, start=1):
    build_bookcase_unit(f"Bookshelf_Right_{index:02d}", +1, center, index + 20)


# ============================================================================
# 9. 中央小组桌：六张梯形桌拼成中空正六边形
# ============================================================================

def regular_hex_vertices(cx, cy, radius, angle_offset_deg=30.0):
    """返回平顶正六边形的六个顶点，顶点按逆时针顺序排列。"""
    return [
        (
            cx + radius * math.cos(math.radians(angle_offset_deg + i * 60.0)),
            cy + radius * math.sin(math.radians(angle_offset_deg + i * 60.0)),
        )
        for i in range(6)
    ]


def build_chair(name, cx, cy, angle):
    seat_r = P["CHAIR_RADIUS"]
    sx = cx + seat_r * math.cos(angle)
    sy = cy + seat_r * math.sin(angle)
    add_box(
        name + "_Seat",
        (0.43, 0.43, 0.085),
        (sx, sy, 0.48),
        MAT["chair"],
        COL["furniture"],
        rotation_z=angle,
        bevel=0.055,
    )
    add_cylinder(
        name + "_Pedestal",
        0.045,
        0.43,
        (sx, sy, 0.235),
        MAT["metal"],
        COL["furniture"],
        vertices=16,
    )
    back_r = seat_r + 0.22
    bx = cx + back_r * math.cos(angle)
    by = cy + back_r * math.sin(angle)
    add_box(
        name + "_Back",
        (0.08, 0.44, 0.44),
        (bx, by, 0.79),
        MAT["chair"],
        COL["furniture"],
        rotation_z=angle,
        bevel=0.055,
    )


def build_table_cluster(name, cx, cy):
    top_z0 = P["TABLE_HEIGHT"] - 0.065
    top_z1 = P["TABLE_HEIGHT"]
    outer_radius = P["TABLE_OUTER_HEX_RADIUS"]
    inner_radius = P["TABLE_INNER_HEX_RADIUS"]
    outer = regular_hex_vertices(cx, cy, outer_radius)
    inner = regular_hex_vertices(cx, cy, inner_radius)
    colors = [MAT["table_white"], MAT["table_yellow"], MAT["table_blue"],
              MAT["table_white"], MAT["table_yellow"], MAT["table_blue"]]

    # 对应两条正六边形边之间的区域恰好是等腰梯形：
    # 外边和内边平行，六张桌无缝围合后留下一个精确的中空正六边形。
    for module in range(6):
        next_module = (module + 1) % 6
        points = [
            outer[module],
            outer[next_module],
            inner[next_module],
            inner[module],
        ]
        top = add_prism(
            name + f"_TrapezoidTable_{module + 1}",
            points,
            top_z0,
            top_z1,
            colors[module],
            COL["furniture"],
        )
        top["status"] = "六张等形梯形桌拼成中空正六边形；桌组数量和尺寸仍为暂定"
        top["outer_hex_radius_m"] = outer_radius
        top["inner_hex_radius_m"] = inner_radius

        # 每张梯形桌各有独立支撑，中央正六边形保持完全中空。
        side_normal_angle = math.radians(60.0 + module * 60.0)
        support_radius = (outer_radius + inner_radius) * math.cos(math.radians(30.0)) / 2.0
        support_x = cx + support_radius * math.cos(side_normal_angle)
        support_y = cy + support_radius * math.sin(side_normal_angle)
        add_cylinder(
            name + f"_TrapezoidTable_{module + 1}_Pedestal",
            0.085,
            P["TABLE_HEIGHT"] - 0.10,
            (support_x, support_y, (P["TABLE_HEIGHT"] - 0.10) / 2),
            MAT["metal"],
            COL["furniture"],
            vertices=20,
        )
        add_cylinder(
            name + f"_TrapezoidTable_{module + 1}_Foot",
            0.20,
            0.035,
            (support_x, support_y, 0.025),
            MAT["metal"],
            COL["furniture"],
            vertices=24,
        )

        # 椅子放在每张梯形桌外侧边的法线方向。
        build_chair(
            name + f"_Chair_{module + 1}",
            cx,
            cy,
            side_normal_angle,
        )


cluster_index = 1
for cy in P["TABLE_CLUSTER_Y"]:
    for cx in P["TABLE_CLUSTER_X"]:
        cluster = f"TableCluster_{cluster_index:02d}"
        build_table_cluster(cluster, cx, cy)
        cluster_index += 1


# ============================================================================
# 10. 顶部人工照明（无窗）
# ============================================================================

if P["PHOTO_DETAILS"]:
    runpy.run_path(str(OUT_DIR / "Tools" / "Blender" / "photo_details.py"), init_globals=globals())

for row, y in enumerate((2.0, 5.3, 8.6, 11.9, 15.8), start=1):
    for column, x in enumerate((-3.75, -1.25, 1.25, 3.75), start=1):
        panel = add_box(
            f"CeilingLightPanel_{row}_{column}",
            (1.20, 0.60, 0.025),
            (x, y, H - 0.0125),
            MAT["light_panel"],
            COL["lights"],
            bevel=0.025,
        )
        # 剖开顶板进行空间核对时，灯具板会在图中像悬浮白条；保留对象供编辑，
        # 但渲染只使用不可见的 Area Light。
        panel.hide_render = True
        panel["note"] = "顶板默认隐藏时，为避免遮挡核对图，本灯具仅保留在可编辑模型中"

# 面光源数量少于灯具数量，控制后台渲染耗时。
for index, (x, y) in enumerate(((-3.0, 5.0), (3.0, 5.0), (-3.0, 10.5),
                                (3.0, 10.5), (0.0, 15.5)), start=1):
    light_data = bpy.data.lights.new(name=f"AreaLight_{index}", type="AREA")
    light_data.energy = 580
    light_data.shape = "RECTANGLE"
    light_data.size = 3.4
    light_data.color = (0.93, 0.96, 1.0)
    light_obj = bpy.data.objects.new(name=f"AreaLight_{index}", object_data=light_data)
    light_obj.location = (mirror_x(x), y, H - 0.18)
    COL["lights"].objects.link(light_obj)

# 柔和太阳光只用于提高白模可读性，不代表地下空间有自然光。
sun_data = bpy.data.lights.new(name="Render_Fill_Sun", type="SUN")
sun_data.energy = 0.65
sun_data.angle = math.radians(24)
sun_obj = bpy.data.objects.new(name="Render_Fill_Sun", object_data=sun_data)
sun_obj.rotation_euler = (math.radians(32), math.radians(-18), math.radians(-28))
COL["lights"].objects.link(sun_obj)


# ============================================================================
# 11. 俯视图标注：把“已确认结构”和“暂定参数”直接画在图上
# ============================================================================

annotation_z = H + 0.12
add_text(
    "Label_Front",
    "前方教学端  FRONT",
    (0.0, -0.72, annotation_z),
    0.36,
    MAT["annotation"],
    COL["annotations"],
)
add_text(
    "Label_Back",
    "后墙：舞台短边直接贴墙  BACK WALL",
    (0.0, L + 0.72, annotation_z),
    0.33,
    MAT["annotation"],
    COL["annotations"],
)
add_text(
    "Label_Curtain",
    "幕布轨道（当前拉开）",
    (0.0, P["CURTAIN_Y"] - 0.33, annotation_z),
    0.27,
    MAT["annotation"],
    COL["annotations"],
)
add_text(
    "Label_Stage",
    "架高木制梯形舞台",
    (0.0, 16.35, annotation_z),
    0.32,
    MAT["annotation"],
    COL["annotations"],
)
add_text(
    "Label_GroupTables",
    "每组：6 张梯形桌拼成中空正六边形（组数暂定）",
    (0.0, 13.35, annotation_z),
    0.22,
    MAT["annotation"],
    COL["annotations"],
)
add_text(
    "Label_LeftProp",
    "左道具间",
    (-5.25, 16.30, annotation_z),
    0.24,
    MAT["annotation"],
    COL["annotations"],
    rotation_z=math.radians(90),
)
add_text(
    "Label_RightProp",
    "右道具间",
    (5.25, 16.30, annotation_z),
    0.24,
    MAT["annotation"],
    COL["annotations"],
    rotation_z=math.radians(90),
)
for text_name, text_body, y in (
    ("Label_DoorFront", "前门 → 讲台", P["FRONT_DOOR_Y"]),
    ("Label_DoorMiddle", "中门 → 最后一排桌", P["MIDDLE_DOOR_Y"]),
    ("Label_DoorRear", "后门 → 右道具间", P["REAR_DOOR_Y"]),
):
    add_text(
        text_name,
        text_body,
        (W / 2 + 1.15, y, annotation_z),
        0.25,
        MAT["annotation"],
        COL["annotations"],
        rotation_z=math.radians(90),
    )

# 总宽度尺寸线
dim_y = -0.30
add_curve_line(
    "Dimension_RoomWidth",
    [(-W / 2, dim_y, annotation_z), (W / 2, dim_y, annotation_z)],
    MAT["annotation"],
    COL["annotations"],
    bevel_depth=0.012,
)
add_curve_line(
    "Dimension_RoomWidth_LeftTick",
    [(-W / 2, dim_y - 0.16, annotation_z), (-W / 2, dim_y + 0.16, annotation_z)],
    MAT["annotation"],
    COL["annotations"],
    bevel_depth=0.012,
)
add_curve_line(
    "Dimension_RoomWidth_RightTick",
    [(W / 2, dim_y - 0.16, annotation_z), (W / 2, dim_y + 0.16, annotation_z)],
    MAT["annotation"],
    COL["annotations"],
    bevel_depth=0.012,
)
add_text(
    "DimensionLabel_RoomWidth",
    f"暂定宽度 {W:.1f} m",
    (0.0, dim_y + 0.22, annotation_z),
    0.24,
    MAT["annotation"],
    COL["annotations"],
)

# 总长度尺寸线
dim_x = -W / 2 - 0.45
add_curve_line(
    "Dimension_RoomLength",
    [(dim_x, 0.0, annotation_z), (dim_x, L, annotation_z)],
    MAT["annotation"],
    COL["annotations"],
    bevel_depth=0.012,
)
add_curve_line(
    "Dimension_RoomLength_FrontTick",
    [(dim_x - 0.16, 0.0, annotation_z), (dim_x + 0.16, 0.0, annotation_z)],
    MAT["annotation"],
    COL["annotations"],
    bevel_depth=0.012,
)
add_curve_line(
    "Dimension_RoomLength_BackTick",
    [(dim_x - 0.16, L, annotation_z), (dim_x + 0.16, L, annotation_z)],
    MAT["annotation"],
    COL["annotations"],
    bevel_depth=0.012,
)
add_text(
    "DimensionLabel_RoomLength",
    f"暂定长度 {L:.1f} m",
    (dim_x - 0.22, L / 2, annotation_z),
    0.24,
    MAT["annotation"],
    COL["annotations"],
    rotation_z=math.radians(90),
)


# ============================================================================
# 12. 相机与渲染
# ============================================================================

def make_camera(name, location, target, lens=46.0, ortho_scale=None):
    name = mirror_side_text(name)
    location = mirror_location(location)
    target = mirror_location(target)
    data = bpy.data.cameras.new(name + "_Data")
    obj = bpy.data.objects.new(name, data)
    COL["cameras"].objects.link(obj)
    obj.location = location
    if ortho_scale is not None:
        data.type = "ORTHO"
        data.ortho_scale = ortho_scale
    else:
        data.type = "PERSP"
        data.lens = lens
    aim_object(obj, target)
    return obj


top_camera = make_camera(
    "Camera_Top_俯视核对",
    (0.0, L / 2, 25.0),
    (0.0, L / 2, 0.0),
    ortho_scale=27.6,
)
stage_camera = make_camera(
    "Camera_Perspective_FrontToStage_从教学端看舞台",
    (-7.9, -2.1, 7.8),
    (0.0, 10.5, 0.95),
    lens=42.0,
)
teaching_camera = make_camera(
    "Camera_Perspective_StageToTeaching_从舞台端看教学墙",
    (7.8, 19.4, 8.3),
    (0.0, 7.0, 0.95),
    lens=43.0,
)

scene.render.image_settings.color_mode = "RGB"

# 俯视图：保留全部外墙和结构标注。
scene.camera = top_camera
COL["annotations"].hide_render = False
front_wall.hide_render = False
back_wall.hide_render = False
left_wall.hide_render = False
for wall_part in right_wall_parts:
    wall_part.hide_render = False
set_render(scene, TOP_RENDER_PATH, 1800, 1350)

# 从前方看舞台：隐藏前墙和左墙，其他几何保持真实位置。
scene.camera = stage_camera
COL["annotations"].hide_render = True
front_wall.hide_render = True
left_wall.hide_render = True
set_render(scene, STAGE_RENDER_PATH, 1800, 1200)

# 从舞台端看教学墙：隐藏后墙和右侧外墙，能同时看到教学墙、桌区、门位。
scene.camera = teaching_camera
back_wall.hide_render = True
for wall_part in right_wall_parts:
    wall_part.hide_render = True
set_render(scene, TEACHING_RENDER_PATH, 1800, 1200)

# 恢复模型的完整渲染状态；默认打开“从前方看舞台”相机。
COL["annotations"].hide_render = False
front_wall.hide_render = False
back_wall.hide_render = False
left_wall.hide_render = False
for wall_part in right_wall_parts:
    wall_part.hide_render = False
scene.camera = stage_camera


# ============================================================================
# 13. 元数据、可编辑文件与通用交换格式
# ============================================================================

scene["model_title"] = "HumanityTrinityRebuild：照片参考细化 v02"
scene["model_status"] = "结构推定模型，不是实测建筑图或施工图"
scene["confirmed_layout"] = (
    "无窗矩形地下教室；前方教学端；中央小组桌；两侧书架柜体；"
    "左侧前中后三门；幕布位于桌区与舞台之间；后部梯形架高木舞台；"
    "舞台短边直接贴后墙；左右角部为密闭道具间；后门直通左道具间。"
)
scene["provisional_parameters_json"] = json.dumps(P, ensure_ascii=False)

notes = bpy.data.texts.new("README_参数与证据说明")
notes.write(
    "本模型结合现场记忆与2026-09-05提供的四张室内照片，尺寸仍为暂定。\n"
    "主要入口：生成脚本顶部 P 参数；细化代码：Tools/Blender/photo_details.py。\n"
    "Y=0为教学墙，Y=ROOM_LENGTH为后墙；三门已镜像到X负侧。\n"
    "每组为六张等形梯形桌围成中空正六边形；九组总数仍是假设。\n"
    "照片确认：浅木色两级舞台、吧台、高白柜、灰绿地板色带、白色脚轮椅和吊顶空调。\n"
    "幕布默认打开，Unreal可缓动开合；屏幕与黑板真实机械关系仍待确认。\n"
    "木纹、地坪与织物贴图为代码生成的近似材质，原始照片未嵌入。\n"
)

OUT_DIR.mkdir(parents=True, exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_PATH))

try:
    bpy.ops.export_scene.gltf(
        filepath=str(GLB_PATH),
        export_format="GLB",
        export_yup=True,
        export_cameras=True,
        export_lights=True,
    )
    print(f"GLB exported: {GLB_PATH}")
except Exception as exc:
    print(f"GLB export skipped because Blender reported: {exc}")

# 保存 GLB 导出后的最终状态。
bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_PATH))

print("=" * 72)
print("HUMANITY_TRINITY_REBUILD MODEL BUILD COMPLETE")
print(f"BLEND: {BLEND_PATH}")
print(f"TOP: {TOP_RENDER_PATH}")
print(f"STAGE VIEW: {STAGE_RENDER_PATH}")
print(f"TEACHING VIEW: {TEACHING_RENDER_PATH}")
print("=" * 72)
