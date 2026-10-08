param(
    [string]$UnrealRoot = 'D:\EpicGames\UE_5.7',
    [string[]]$Cases = @('walk-safe', 'walk-auto', 'walk-exposed', 'walk-lights', 'walk-screen', 'walk-late', 'walk-exit', 'seek-safe', 'hide-safe', 'practice-safe'),
    [switch]$Capture
)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Project = Join-Path $ProjectRoot 'UnrealProject\HumanityTrinityRebuild.uproject'
$Editor = Join-Path $UnrealRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Logs = Join-Path $ProjectRoot 'UnrealProject\Saved\Logs'
New-Item -ItemType Directory -Path $Logs -Force | Out-Null
foreach ($Case in $Cases) {
    $Pieces = $Case.Split('-')
    if ($Pieces.Count -ne 2) { throw "Invalid validation case: $Case" }
    $Mode, $Scenario = $Pieces
    $Map = '/Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildWalkthrough'
    $Options = @()
    if ($Mode -in @('seek', 'hide', 'practice')) {
        $Map = '/Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildHideAndSeek'
    }
    if ($Mode -eq 'hide') { $Options += '-PlayerHides' }
    if ($Mode -eq 'practice') { $Options += '-HideAndSeekPractice' }
    if ($Capture) { $Options += '-TeacherPatrolCapture' }
    $Log = Join-Path $Logs ('TeacherPatrol-' + $Case + '.log')
    $Arguments = @(('"' + $Project + '"'), $Map, '-game', '-RenderOffscreen', '-ResX=1280', '-ResY=720',
        '-unattended', '-nosound', '-TeacherPatrolSelfTest', ('-TeacherPatrolScenario=' + $Scenario),
        ('-abslog="' + $Log + '"')) + $Options
    $Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -WindowStyle Hidden -PassThru
    if (-not $Process.WaitForExit(240000)) {
        Stop-Process -Id $Process.Id
        throw "Validation timed out: $Case. Inspect $Log"
    }
    if (-not (Test-Path -LiteralPath $Log)) { throw "No validation log: $Case" }
    $Content = Get-Content -LiteralPath $Log -Raw
    $Completed = $Content -match '\[TEACHER_PATROL_SELFTEST\] PASS scenario='
    if ($Scenario -eq 'exit') {
        $Completed = $Content -match '\[TEACHER_PATROL_SELFTEST\] exit_pending=PASS' -and $Content -match '\[TEACHER_PATROL\] FAILURE_EXIT'
    }
    if (-not $Completed -or
        $Content -match '\[TEACHER_PATROL_SELFTEST\].*=FAIL|\[TEACHER_PATROL_SELFTEST\] FAIL scenario=') {
        throw "Patrol validation failed: $Case. Inspect $Log"
    }
    Write-Output "PASS $Case : $Log"
}
