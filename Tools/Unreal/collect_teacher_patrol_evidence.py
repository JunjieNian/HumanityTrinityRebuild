"""Collect only explicit runtime assertions; never infer success from exit code."""
from pathlib import Path
import hashlib
import json
import re
import shutil

root = Path(__file__).resolve().parents[2]
logs = root / 'UnrealProject' / 'Saved' / 'Logs'
out = root / 'Docs' / 'Validation'
out.mkdir(parents=True, exist_ok=True)
cases = ['walk-safe', 'walk-auto', 'walk-exposed', 'walk-lights', 'walk-screen',
         'walk-late', 'walk-exit', 'seek-safe', 'hide-safe', 'practice-safe']
expected = {case: '[TEACHER_PATROL_SELFTEST] PASS scenario=' for case in cases}
expected['walk-exit'] = '[TEACHER_PATROL] FAILURE_EXIT'
expected.update({'SeekRegression': '[HIDE_AND_SEEK_SELFTEST] PASS',
                 'HideRegression': '[PLAYER_HIDING_SELFTEST] PASS',
                 'RoomRegression': '[HUMANITY_TRINITY_REBUILD_SELFTEST] PASS',
                 'FullRoomRegression': '[HUMANITY_TRINITY_REBUILD_SELFTEST] PASS',
                 'MenuRegression': '[MODE_MENU_SELFTEST] walkthrough=PASS'})
summary = []
evidence = ['Teacher patrol verification, 2026-10-07. Unreal Engine 5.7.\n']
for case, marker in expected.items():
    source = logs / f'TeacherPatrol-{case}.log'
    content = source.read_text(encoding='utf-8', errors='replace')
    assert marker in content, f'Missing PASS marker: {case}'
    assert not re.search(r'\[(?:TEACHER_PATROL_SELFTEST|HIDE_AND_SEEK_SELFTEST|PLAYER_HIDING_SELFTEST|HUMANITY_TRINITY_REBUILD_SELFTEST|MODE_MENU_SELFTEST)\][^\n]*\bFAIL\b', content), f'Failed assertion: {case}'
    if case == 'walk-exit':
        assert '[TEACHER_PATROL_SELFTEST] exit_pending=PASS' in content
    if case == 'MenuRegression':
        assert '[MODE_MENU_SELFTEST] teacher_checkbox=PASS preference_restored=YES' in content
    checks = [line for line in content.splitlines() if re.search(r'\[(?:\w*SELFTEST|TEACHER_PATROL)\]', line)]
    evidence.extend([f'\n--- {case} ---', *checks])
    summary.append({'case': case, 'result': 'PASS', 'log_sha256': hashlib.sha256(source.read_bytes()).hexdigest()})
(out / 'TeacherPatrolRuntime.txt').write_text('\n'.join(evidence) + '\n', encoding='utf-8')
setup = (logs / 'TeacherPatrolSetup.log').read_text(encoding='utf-8', errors='replace')
assert '[TEACHER_PATROL_SETUP] COMPLETE environment=YES teacher=YES audio=YES' in setup
summary.append({'case': 'asset_import', 'result': 'PASS'})
(out / 'TeacherPatrolResults.json').write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
previews = root / 'Docs' / 'Previews'
screens = root / 'UnrealProject' / 'Saved' / 'Screenshots'
for name in ['TeacherPatrol_walk_safe_Closed.png', 'TeacherPatrol_walk_safe_Warning.png',
             'TeacherPatrol_walk_safe_Inspection.png', 'TeacherPatrol_walk_exposed_Failed.png', 'ModeMenu.png']:
    target = 'TeacherPatrol_Menu.png' if name == 'ModeMenu.png' else name
    shutil.copy2(screens / name, previews / target)
print(f'COLLECTED {len(summary)} verified results, 5 screenshots')
