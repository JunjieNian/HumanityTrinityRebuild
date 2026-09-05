param(
    [string]$UnrealRoot = $env:UE_ROOT
)

$ErrorActionPreference = 'Stop'

$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$UProject = Join-Path $ProjectRoot 'UnrealProject\HumanityTrinityRebuild.uproject'
$SetupScript = Join-Path $ProjectRoot 'Tools\Unreal\setup_humanity_trinity_rebuild_unreal.py'

$Candidates = @()
if ($UnrealRoot) {
    $Candidates += $UnrealRoot
}
$Candidates += @(
    'D:\EpicGames\UE_5.7',
    'C:\Program Files\Epic Games\UE_5.7'
)

$ResolvedUnrealRoot = $Candidates |
    Where-Object { Test-Path -LiteralPath (Join-Path $_ 'Engine\Build\BatchFiles\Build.bat') } |
    Select-Object -First 1

if (-not $ResolvedUnrealRoot) {
    throw 'Unreal Engine 5.7 was not found. Set UE_ROOT to the UE_5.7 installation directory.'
}

$BuildBat = Join-Path $ResolvedUnrealRoot 'Engine\Build\BatchFiles\Build.bat'
$EditorCmd = Join-Path $ResolvedUnrealRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

& $BuildBat HumanityTrinityRebuildEditor Win64 Development "-Project=$UProject" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) {
    throw "Unreal C++ build failed with exit code $LASTEXITCODE"
}

& $EditorCmd $UProject "-ExecutePythonScript=$SetupScript" -unattended -nop4 -nosplash -nullrhi
if ($LASTEXITCODE -ne 0) {
    throw "Unreal setup script failed with exit code $LASTEXITCODE"
}
$ProjectLog = Join-Path $ProjectRoot 'UnrealProject\Saved\Logs\HumanityTrinityRebuild.log'
if (-not (Select-String -LiteralPath $ProjectLog -Pattern '\[HUMANITY_TRINITY_REBUILD_SETUP\] SETUP_COMPLETE' -Quiet)) {
    throw 'Unreal did not confirm a complete asset setup. See the project log for the Python error.'
}

Write-Output "HumanityTrinityRebuild build and setup completed with $ResolvedUnrealRoot."
