"""
Create a clean, combined Unreal runtime asset from the editable Blender model.

Run:
    blender.exe --background HumanityTrinityRebuild.blend \
        --python Tools/Blender/export_humanity_trinity_rebuild.py

The source .blend is never saved by this script.
"""

from pathlib import Path
import bpy


PROJECT_ROOT = Path(__file__).resolve().parents[2]
OUTPUT_PATH = (
    PROJECT_ROOT
    / "UnrealProject"
    / "Content"
    / "SourceAssets"
    / "HumanityTrinityRebuildEnvironment_Runtime.glb"
)

EXCLUDED_COLLECTION_PREFIXES = (
    "06_Lighting",
    "07_Annotations",
    "99_Cameras",
)


def object_is_excluded(obj):
    if obj.type in {"LIGHT", "CAMERA"}:
        return True
    for collection in obj.users_collection:
        if collection.name.startswith(EXCLUDED_COLLECTION_PREFIXES):
            return True
    return False


for obj in bpy.context.scene.objects:
    obj.hide_set(False)
    obj.hide_viewport = False
    obj.hide_render = False

bpy.ops.object.select_all(action="DESELECT")

runtime_objects = []
for obj in list(bpy.context.scene.objects):
    if object_is_excluded(obj):
        continue
    if obj.type in {"MESH", "CURVE", "FONT", "SURFACE"}:
        obj.select_set(True)
        runtime_objects.append(obj)

if not runtime_objects:
    raise RuntimeError("No runtime geometry was found in the source blend file.")

# Convert text/curves and evaluate modifiers before joining.
bpy.context.view_layer.objects.active = runtime_objects[0]
bpy.ops.object.convert(target="MESH")

selected_meshes = [obj for obj in bpy.context.selected_objects if obj.type == "MESH"]
if not selected_meshes:
    raise RuntimeError("Runtime objects could not be converted to meshes.")

floor_candidates = [obj for obj in selected_meshes if obj.name.startswith("Floor_")]
bpy.context.view_layer.objects.active = floor_candidates[0] if floor_candidates else selected_meshes[0]
bpy.ops.object.join()

environment = bpy.context.object
environment.name = "HumanityTrinityRebuildEnvironment_Runtime"
environment.data.name = "HumanityTrinityRebuildEnvironment_Runtime_Mesh"

# Apply transforms so Unreal receives predictable centimeters and a stable pivot.
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

dimensions = tuple(round(value, 4) for value in environment.dimensions)
if not (11.0 <= dimensions[0] <= 13.5 and 17.0 <= dimensions[1] <= 19.5):
    raise RuntimeError(f"Unexpected environment bounds in Blender meters: {dimensions}")

OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
bpy.ops.export_scene.gltf(
    filepath=str(OUTPUT_PATH),
    export_format="GLB",
    use_selection=True,
    export_yup=True,
    export_apply=True,
    export_cameras=False,
    export_lights=False,
    export_extras=True,
)

print("=" * 72)
print("UNREAL RUNTIME EXPORT COMPLETE")
print(f"Object: {environment.name}")
print(f"Bounds in Blender meters: {dimensions}")
print(f"Materials: {len(environment.data.materials)}")
print(f"Output: {OUTPUT_PATH}")
print("=" * 72)
