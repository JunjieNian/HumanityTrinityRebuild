"""
Unreal Editor automation for the Humanity Trinity walkthrough prototype.

It imports the combined GLB, enables complex collision, creates the runtime
level, adds a PlayerStart, and saves all generated assets.
"""

from pathlib import Path
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


def main():
    log(f"Project directory: {PROJECT_DIR}")
    mesh = import_environment()
    create_walkthrough_level()
    log(f"SETUP_COMPLETE mesh={mesh.get_path_name()} level={LEVEL_PATH}")


main()
