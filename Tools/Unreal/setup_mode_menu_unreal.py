"""Create a separate 3D mode-selection map from the validated walkthrough."""

import unreal


SOURCE_MAP = "/Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildWalkthrough"
MENU_MAP = "/Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildModeMenu"
GAME_MODE_CLASS = "/Script/HumanityTrinityRebuild.HumanityTrinityRebuildModeMenuGameMode"


def main():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.load_level(SOURCE_MAP):
        raise RuntimeError(f"Missing source room: {SOURCE_MAP}")
    if unreal.EditorAssetLibrary.does_asset_exist(MENU_MAP):
        unreal.EditorAssetLibrary.delete_asset(MENU_MAP)
    duplicate = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP, MENU_MAP)
    if not duplicate or not unreal.EditorAssetLibrary.save_asset(MENU_MAP):
        raise RuntimeError(f"Could not create {MENU_MAP}")
    del duplicate
    unreal.SystemLibrary.collect_garbage()
    if not levels.load_level(MENU_MAP):
        raise RuntimeError(f"Could not load {MENU_MAP}")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    game_mode = unreal.load_class(None, GAME_MODE_CLASS)
    if not game_mode:
        raise RuntimeError(f"Missing compiled class: {GAME_MODE_CLASS}")
    world.get_world_settings().set_editor_property("default_game_mode", game_mode)
    if not levels.save_current_level():
        raise RuntimeError("Could not save menu map")
    unreal.log(f"[MODE_MENU_SETUP] COMPLETE map={MENU_MAP} game_mode={game_mode.get_path_name()}")


main()
