@echo off
setlocal

set "PROJECT_FILE=%~dp0UnrealProject\HumanityTrinityRebuild.uproject"
if defined UE_ROOT set "UE_EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
if not defined UE_EDITOR if exist "D:\EpicGames\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" set "UE_EDITOR=D:\EpicGames\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
if not defined UE_EDITOR if exist "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" set "UE_EDITOR=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%PROJECT_FILE%" (
    echo Project file not found: %PROJECT_FILE%
    pause
    exit /b 1
)
if not exist "%UE_EDITOR%" (
    echo Unreal Engine 5.7 was not found. Set UE_ROOT to the UE_5.7 installation directory.
    pause
    exit /b 1
)

start "" "%UE_EDITOR%" "%PROJECT_FILE%" /Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildWalkthrough -game -windowed -ResX=1600 -ResY=900
endlocal
