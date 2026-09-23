"""Import the joint-local character without modifying either room map."""
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).parent
destination = '/Game/HumanityTrinityRebuild/Characters/Hider'
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
sources = sorted((root / 'Assets' / 'Hider').glob('SM_Hider_*.glb'))
if len(sources) != 9:
    raise RuntimeError('Generate all nine character meshes with Tools/Blender/build_hider.py first')
for source in sources:
    name = source.stem
    path = f'{destination}/{name}'
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(source))
    task.set_editor_property('destination_path', destination + '/Import_' + name)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    asset_tools.import_asset_tasks([task])
    meshes = [unreal.EditorAssetLibrary.load_asset(p) for p in task.imported_object_paths]
    meshes = [a for a in meshes if isinstance(a, unreal.StaticMesh)]
    if len(meshes) != 1:
        raise RuntimeError(f'{name}: expected one mesh, got {len(meshes)}')
    mesh = meshes[0]
    if mesh.get_path_name().split('.')[0] != path:
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.EditorAssetLibrary.delete_asset(path)
        if not asset_tools.rename_assets([unreal.AssetRenameData(mesh, destination, name)]):
            raise RuntimeError(f'{name}: could not normalize mesh path')
    mesh.get_editor_property('body_setup').set_editor_property(
        'collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.log(f'[HIDER_IMPORT] {name} bounds={mesh.get_bounds().box_extent}')
unreal.EditorAssetLibrary.save_directory(destination)
unreal.log('[HIDER_IMPORT] COMPLETE')
