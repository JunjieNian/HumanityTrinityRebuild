"""Create a separate game map and import the reproducible footstep sound."""

from pathlib import Path
import unreal


PROJECT_DIR = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
WALKTHROUGH = "/Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildWalkthrough"
GAME_MAP = "/Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildHideAndSeek"
SOUND_PATH = "/Game/HumanityTrinityRebuild/Audio/SW_HiderFootstep"


def log(message):
    unreal.log(f"[HIDE_AND_SEEK_SETUP] {message}")


def create_hider_material():
    path = "/Game/HumanityTrinityRebuild/Materials/M_Hider"
    material = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_Hider", "/Game/HumanityTrinityRebuild/Materials",
            unreal.Material, unreal.MaterialFactoryNew())
    editing = unreal.MaterialEditingLibrary
    editing.delete_all_material_expressions(material)
    color = editing.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -300, 0)
    color.set_editor_property("constant", unreal.LinearColor(0.07, 0.10, 0.11, 1.0))
    editing.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = editing.create_material_expression(material, unreal.MaterialExpressionConstant, -300, 150)
    rough.set_editor_property("r", 0.85)
    editing.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    editing.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    log(f"MATERIAL_READY {path}")


def main():
    audio_file = PROJECT_DIR.parent / "Assets" / "Audio" / "hider_footstep.wav"
    if not audio_file.exists():
        raise RuntimeError(f"Generate the footstep first: {audio_file}")
    if not unreal.EditorAssetLibrary.does_asset_exist(WALKTHROUGH):
        raise RuntimeError(f"Missing walkthrough map: {WALKTHROUGH}")
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not subsystem.load_level(WALKTHROUGH):
        raise RuntimeError(f"Could not load source map: {WALKTHROUGH}")

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(audio_file))
    task.set_editor_property("destination_path", "/Game/HumanityTrinityRebuild/Audio")
    task.set_editor_property("destination_name", "SW_HiderFootstep")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.EditorAssetLibrary.load_asset(SOUND_PATH)
    if not isinstance(sound, unreal.SoundWave):
        raise RuntimeError(f"Footstep did not import: {SOUND_PATH}")
    log(f"AUDIO_READY {sound.get_path_name()}")
    create_hider_material()

    if unreal.EditorAssetLibrary.does_asset_exist(GAME_MAP):
        unreal.EditorAssetLibrary.delete_asset(GAME_MAP)
    duplicate = unreal.EditorAssetLibrary.duplicate_asset(WALKTHROUGH, GAME_MAP)
    if not duplicate:
        raise RuntimeError(f"Could not duplicate {WALKTHROUGH}")
    if not unreal.EditorAssetLibrary.save_asset(GAME_MAP):
        raise RuntimeError(f"Could not save {GAME_MAP}")
    # A Python reference to the newly duplicated UWorld blocks level loading's
    # garbage-collection check, even though the asset itself is valid.
    del duplicate
    unreal.SystemLibrary.collect_garbage()
    if not subsystem.load_level(GAME_MAP):
        raise RuntimeError(f"Could not load {GAME_MAP}")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    game_mode = unreal.load_class(None,
        "/Script/HumanityTrinityRebuild.HumanityTrinityRebuildHideAndSeekGameMode")
    if not game_mode:
        raise RuntimeError("Hide-and-seek game mode C++ class is unavailable")
    world.get_world_settings().set_editor_property("default_game_mode", game_mode)
    subsystem.save_current_level()
    log(f"SETUP_COMPLETE map={GAME_MAP} mode={game_mode.get_path_name()}")


main()
