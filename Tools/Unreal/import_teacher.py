"""Import the editable articulated patrol teacher at stable runtime paths."""
from pathlib import Path
import unreal
root=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).parent
destination='/Game/HumanityTrinityRebuild/Characters/Teacher'
asset_tools=unreal.AssetToolsHelpers.get_asset_tools()
sources=sorted((root/'Assets'/'Teacher').glob('SM_Teacher_*.glb'))
if len(sources)!=9:
    raise RuntimeError('Generate nine teacher meshes with Tools/Blender/build_teacher.py first')
for source in sources:
    name=source.stem; path=f'{destination}/{name}'
    task=unreal.AssetImportTask(); task.filename=str(source)
    task.destination_path=destination+'/Import_'+name
    task.automated=True; task.replace_existing=True; task.save=True
    asset_tools.import_asset_tasks([task])
    meshes=[unreal.EditorAssetLibrary.load_asset(p) for p in task.imported_object_paths]
    meshes=[a for a in meshes if isinstance(a,unreal.StaticMesh)]
    if len(meshes)!=1: raise RuntimeError(f'{name}: expected one mesh, got {len(meshes)}')
    mesh=meshes[0]
    if mesh.get_path_name().split('.')[0]!=path:
        if unreal.EditorAssetLibrary.does_asset_exist(path): unreal.EditorAssetLibrary.delete_asset(path)
        if not asset_tools.rename_assets([unreal.AssetRenameData(mesh,destination,name)]):
            raise RuntimeError(f'{name}: could not normalize mesh path')
    nanite=mesh.get_editor_property('nanite_settings'); nanite.set_editor_property('enabled',False)
    mesh.set_editor_property('nanite_settings',nanite)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.log(f'[TEACHER_IMPORT] {name} bounds={mesh.get_bounds().box_extent}')
unreal.EditorAssetLibrary.save_directory(destination)
unreal.log('[TEACHER_IMPORT] COMPLETE meshes=9')
