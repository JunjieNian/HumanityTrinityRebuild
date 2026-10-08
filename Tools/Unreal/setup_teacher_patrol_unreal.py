"""Import classroom, teacher and audio for a complete patrol-feature rebuild."""
from pathlib import Path
import runpy
import unreal

scripts = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).parent / 'Tools' / 'Unreal'
for name in ('setup_humanity_trinity_rebuild_unreal.py', 'import_teacher.py', 'import_teacher_audio.py'):
    runpy.run_path(str(scripts / name), run_name='__main__')
unreal.log('[TEACHER_PATROL_SETUP] COMPLETE environment=YES teacher=YES audio=YES')
