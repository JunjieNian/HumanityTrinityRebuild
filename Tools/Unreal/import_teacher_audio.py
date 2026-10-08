"""Import reproducible patrol speech and effects without modifying maps."""
from pathlib import Path
import unreal

root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).parent
destination = '/Game/HumanityTrinityRebuild/Audio'
names = ['SW_TeacherFootstep', 'SW_TeacherKeys', 'SW_TeacherWarning', 'SW_TeacherClear', 'SW_TeacherCaught']
for name in names:
    source = root / 'Assets' / 'Audio' / 'Teacher' / (name + '.wav')
    if not source.exists():
        raise RuntimeError(f'Missing sound; run Tools/Audio/generate_teacher_audio.ps1: {source}')
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(source))
    task.set_editor_property('destination_path', destination)
    task.set_editor_property('destination_name', name)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.EditorAssetLibrary.load_asset(destination + '/' + name)
    if not isinstance(sound, unreal.SoundWave):
        raise RuntimeError(f'Invalid imported audio: {name}')
    unreal.log(f'[TEACHER_AUDIO_IMPORT] {name} duration={sound.get_editor_property("duration"):.3f}')
unreal.log('[TEACHER_AUDIO_IMPORT] COMPLETE sounds=5')
