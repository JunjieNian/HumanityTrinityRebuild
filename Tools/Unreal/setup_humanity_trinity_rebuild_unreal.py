"""
Unreal Editor automation for the Humanity Trinity walkthrough prototype.

It imports the combined GLB, enables complex collision, creates the runtime
level, adds a PlayerStart, and saves all generated assets.
"""

from pathlib import Path
import json
import struct
import unreal


PROJECT_DIR = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
SOURCE_GLB = PROJECT_DIR / "Content" / "SourceAssets" / "HumanityTrinityRebuildEnvironment_Runtime.glb"
DESTINATION_PATH = "/Game/HumanityTrinityRebuild/Environment"
EXPECTED_MESH_PATH = f"{DESTINATION_PATH}/HumanityTrinityRebuildEnvironment_Runtime"
LEVEL_PATH = "/Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildWalkthrough"


def log(message):
    unreal.log(f"[HUMANITY_TRINITY_REBUILD_SETUP] {message}")


def import_environment():
    if not SOURCE_GLB.exists():
        raise RuntimeError(f"Missing runtime GLB: {SOURCE_GLB}")

    if unreal.EditorAssetLibrary.does_asset_exist(EXPECTED_MESH_PATH):
        unreal.EditorAssetLibrary.delete_asset(EXPECTED_MESH_PATH)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(SOURCE_GLB))
    task.set_editor_property("destination_path", DESTINATION_PATH)
    task.set_editor_property("destination_name", "HumanityTrinityRebuildEnvironment_Runtime")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset_tools.import_asset_tasks([task])

    imported_paths = list(task.get_editor_property("imported_object_paths"))
    log(f"Import task returned {len(imported_paths)} assets")
    for path in imported_paths:
        log(f"  imported: {path}")

    candidates = imported_paths + list(
        unreal.EditorAssetLibrary.list_assets(
            DESTINATION_PATH,
            recursive=True,
            include_folder=False,
        )
    )

    static_mesh = None
    for asset_path in candidates:
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if isinstance(asset, unreal.StaticMesh):
            static_mesh = asset
            break

    if static_mesh is None:
        raise RuntimeError(
            "The GLB import completed but no StaticMesh was found under "
            f"{DESTINATION_PATH}. Imported paths: {imported_paths}"
        )

    if static_mesh.get_path_name().split(".")[0] != EXPECTED_MESH_PATH:
        rename_data = unreal.AssetRenameData(
            static_mesh,
            DESTINATION_PATH,
            "HumanityTrinityRebuildEnvironment_Runtime",
        )
        if not asset_tools.rename_assets([rename_data]):
            raise RuntimeError(
                "Could not move the imported mesh to the stable runtime path "
                f"{EXPECTED_MESH_PATH}"
            )
        static_mesh = unreal.EditorAssetLibrary.load_asset(EXPECTED_MESH_PATH)
        if static_mesh is None:
            raise RuntimeError(f"Renamed mesh could not be loaded: {EXPECTED_MESH_PATH}")
    body_setup = static_mesh.get_editor_property("body_setup")
    if body_setup:
        body_setup.set_editor_property(
            "collision_trace_flag",
            unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE,
        )

    static_mesh.set_editor_property("allow_cpu_access", True)
    # This entire detailed room is one mesh. Automatic Nanite fallback reduced
    # it to ~6k triangles on SM5, collapsing chair backs and table frames.
    # Keep the original triangles on every supported RHI for reliable geometry.
    nanite = static_mesh.get_editor_property("nanite_settings")
    nanite.set_editor_property("enabled", False)
    static_mesh.set_editor_property("nanite_settings", nanite)
    unreal.EditorAssetLibrary.save_loaded_asset(static_mesh, only_if_is_dirty=False)

    bounds = static_mesh.get_bounds()
    log(
        "Environment bounds in Unreal centimeters: "
        f"origin={bounds.origin}, extent={bounds.box_extent}"
    )
    return static_mesh


def create_walkthrough_level():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        if not level_subsystem.load_level(LEVEL_PATH):
            raise RuntimeError(f"Could not load existing level: {LEVEL_PATH}")
        log(f"Loaded existing level: {LEVEL_PATH}")
    else:
        if not level_subsystem.new_level(LEVEL_PATH):
            raise RuntimeError(f"Could not create level: {LEVEL_PATH}")
        log(f"Created level: {LEVEL_PATH}")

    for actor in actor_subsystem.get_all_level_actors():
        if isinstance(actor, unreal.PlayerStart):
            actor_subsystem.destroy_actor(actor)

    player_start = actor_subsystem.spawn_actor_from_class(
        unreal.PlayerStart,
        unreal.Vector(0.0, -220.0, 100.0),
        unreal.Rotator(0.0, -90.0, 0.0),
    )
    player_start.set_actor_label("PlayerStart_FrontDoor")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world_settings = world.get_world_settings()
    game_mode_class = unreal.load_class(
        None,
        "/Script/HumanityTrinityRebuild.HumanityTrinityRebuildGameMode",
    )
    if game_mode_class:
        world_settings.set_editor_property("default_game_mode", game_mode_class)

    level_subsystem.save_current_level()
    unreal.EditorAssetLibrary.save_directory("/Game/HumanityTrinityRebuild", only_if_is_dirty=False, recursive=True)
    log(f"Saved walkthrough level: {LEVEL_PATH}")


def create_runtime_materials():
    """Real emissive graphs: dynamic parameters must be connected, not just named."""
    editing = unreal.MaterialEditingLibrary
    for name in ("M_PanelLight", "M_TeachingDisplay"):
        path = f"/Game/HumanityTrinityRebuild/Materials/{name}"
        material = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        if material is None:
            material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                name, "/Game/HumanityTrinityRebuild/Materials", unreal.Material, unreal.MaterialFactoryNew())
        editing.delete_all_material_expressions(material)
        color = editing.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -500, 0)
        color.set_editor_property("parameter_name", "Color")
        color.set_editor_property("default_value", unreal.LinearColor(.8, .8, .8, 1))
        intensity = editing.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 180)
        intensity.set_editor_property("parameter_name", "Emission")
        intensity.set_editor_property("default_value", 0.0)
        multiply = editing.create_material_expression(material, unreal.MaterialExpressionMultiply, -200, 130)
        editing.connect_material_expressions(color, "", multiply, "A")
        editing.connect_material_expressions(intensity, "", multiply, "B")
        editing.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
        editing.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        rough = editing.create_material_expression(material, unreal.MaterialExpressionConstant, -200, 300)
        rough.set_editor_property("r", .52 if name == "M_PanelLight" else .26)
        editing.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        editing.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        log(f"Runtime emissive material ready: {path}")


def import_curtain():
    source = SOURCE_GLB.parent / "SM_CurtainPanel.glb"
    if not source.exists():
        raise RuntimeError(f"Missing interactive curtain export: {source}")
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", "/Game/HumanityTrinityRebuild/Interactive")
    task.set_editor_property("destination_name", "SM_CurtainPanel")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    expected = "/Game/HumanityTrinityRebuild/Interactive/SM_CurtainPanel"
    mesh = unreal.EditorAssetLibrary.load_asset(expected) if unreal.EditorAssetLibrary.does_asset_exist(expected) else None
    if mesh is None:
        for path in task.get_editor_property("imported_object_paths"):
            candidate = unreal.EditorAssetLibrary.load_asset(path)
            if isinstance(candidate, unreal.StaticMesh):
                mesh = candidate
                break
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("Interactive curtain mesh did not import at its stable asset path")
    if mesh.get_path_name().split(".")[0] != expected:
        renamed = unreal.AssetToolsHelpers.get_asset_tools().rename_assets([
            unreal.AssetRenameData(mesh, "/Game/HumanityTrinityRebuild/Interactive", "SM_CurtainPanel")])
        if not renamed:
            raise RuntimeError("Could not normalize the imported curtain mesh path")
    nanite = mesh.get_editor_property("nanite_settings")
    nanite.set_editor_property("enabled", False)
    mesh.set_editor_property("nanite_settings", nanite)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    log(f"Curtain bounds: {mesh.get_bounds()}")


def bind_portable_surface_materials(source, mesh_path, import_root):
    """Build explicit PBR graphs; Interchange otherwise reuses old flat materials."""
    data = source.read_bytes()
    json_size = struct.unpack_from("<I", data, 12)[0]
    gltf = json.loads(data[20:20+json_size])
    editing = unreal.MaterialEditingLibrary
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    materials = {}

    def texture(info):
        index = gltf["textures"][info["index"]]["source"]
        name = gltf["images"][index]["name"]
        path = f"{import_root}/Textures/{name}"
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(asset, unreal.Texture2D):
            raise RuntimeError(f"Missing imported texture: {path}")
        return asset

    for spec in gltf["materials"]:
        name = "M_Surface_" + spec["name"].removeprefix("MAT_")
        folder = "/Game/HumanityTrinityRebuild/Materials/Surfaces"
        path = folder + "/" + name
        mat = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        if mat is None:
            mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
        editing.delete_all_material_expressions(mat)
        mat.set_editor_property("two_sided", spec.get("doubleSided", False))
        pbr = spec.get("pbrMetallicRoughness", {})
        if "baseColorTexture" in pbr:
            color = editing.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -400, 0)
            color.set_editor_property("texture", texture(pbr["baseColorTexture"]))
            editing.connect_material_property(color, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
        else:
            color = editing.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
            rgb = pbr.get("baseColorFactor", [1,1,1,1])
            color.set_editor_property("constant", unreal.LinearColor(*rgb))
            editing.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
        for key, prop, default in (("roughnessFactor", unreal.MaterialProperty.MP_ROUGHNESS, 1),
                                   ("metallicFactor", unreal.MaterialProperty.MP_METALLIC, 0)):
            expr = editing.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 220 if key.startswith("rough") else 320)
            expr.set_editor_property("r", pbr.get(key, default))
            editing.connect_material_property(expr, "", prop)
        if "normalTexture" in spec:
            normal = editing.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -400, 420)
            normal.set_editor_property("texture", texture(spec["normalTexture"]))
            normal.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
            editing.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)
        editing.recompile_material(mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mat)
        materials[spec["name"]] = mat

    for index, slot in enumerate(mesh.get_editor_property("static_materials")):
        imported_name = str(slot.get_editor_property("imported_material_slot_name"))
        existing = slot.get_editor_property("material_interface")
        source_name = imported_name if imported_name in materials else (existing.get_name() if existing else "")
        if source_name not in materials:
            raise RuntimeError(f"Unmatched material slot: {source_name}")
        mesh.set_material(index, materials[source_name])
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    log(f"Explicit PBR binding ready: {mesh_path}, {len(materials)} materials")


def main():
    log(f"Project directory: {PROJECT_DIR}")
    mesh = import_environment()
    import_curtain()
    create_runtime_materials()
    bind_portable_surface_materials(SOURCE_GLB, EXPECTED_MESH_PATH,
        DESTINATION_PATH + "/HumanityTrinityRebuildEnvironment_Runtime")
    bind_portable_surface_materials(SOURCE_GLB.parent / "SM_CurtainPanel.glb",
        "/Game/HumanityTrinityRebuild/Interactive/SM_CurtainPanel",
        "/Game/HumanityTrinityRebuild/Interactive/SM_CurtainPanel")
    create_walkthrough_level()
    log(f"SETUP_COMPLETE mesh={mesh.get_path_name()} level={LEVEL_PATH}")


main()
