# HumanityTrinityRebuild

An editable Blender reconstruction and Unreal Engine interactive walkthrough of the **Humanity Trinity Space** (三一人文空间), located on B1 of Duzhi Building at the High School Affiliated to Fudan University.

The repository combines a parameter-driven spatial model with a native Unreal Engine first-person prototype. It is intended for iterative reconstruction: dimensions that are not yet known are isolated as editable parameters instead of being presented as measured facts.

> This is a research-based reconstruction assembled from on-site recollection and publicly available contextual material. It is not an official architectural, construction, fire-safety, or survey drawing.

![Perspective toward the stage](HumanityTrinityRebuild_PerspectiveToStage.png)

## What is included

- Editable Blender source model and a parameterized Python generator.
- A general-purpose GLB export of the full editable scene.
- A merged runtime GLB for Unreal import.
- Unreal Engine 5.7 native C++ first-person movement and interaction.
- Nine movable classroom lights in four independently switchable zones.
- A wall-mounted light switch operated with a camera-center visibility trace.
- Slow dark adaptation and faster bright adaptation through manual exposure control.
- Directional door-leak and display-standby residual light assumptions for the almost-black state.
- Complex collision for the imported classroom, furniture, stage, shelves, and walls.
- Runtime self-tests for the lighting state, adaptation behavior, and real player-view switch trace.

## Confirmed spatial relationships represented by the model

- A sealed, windowless rectangular basement classroom.
- Teaching wall, large display, and podium at the front.
- Central collaborative table area.
- Shelves and storage cabinets along both sides.
- Three doors on the left wall.
- Curtain between the table area and the stage.
- Raised wooden trapezoidal stage, with its long edge facing the classroom and its short edge touching the rear wall.
- Two enclosed triangular prop rooms occupying the corners left by the stage slopes.
- Each collaborative table cluster consists of six congruent trapezoid tables forming a hollow regular hexagon.

Unknown dimensions, fixture specifications, exact switch placement, light grouping, and residual-light sources remain explicitly provisional.

The complete scene is mirrored across the longitudinal centerline through the generator parameter `MIRROR_ACROSS_LONGITUDINAL_AXIS=True`. This records the corrected on-site orientation: when facing the rear stage, the three doors are on the left wall rather than the right.

## Quick start on Windows / 快速开始

Requirements:

- Unreal Engine 5.7.
- Visual Studio 2022 with the Desktop development with C++ and Game development with C++ workloads, if rebuilding the native module.
- Blender 4.5 LTS, if regenerating the model.

To enter the walkthrough directly, run:

```text
Launch_HumanityTrinityRebuild_Walkthrough.cmd
```

To open the Unreal Editor project, run:

```text
Open_HumanityTrinityRebuild_UnrealProject.cmd
```

The launchers first use the `UE_ROOT` environment variable when it is defined, then check these common locations:

```text
D:\EpicGames\UE_5.7
C:\Program Files\Epic Games\UE_5.7
```

If Unreal is elsewhere, set `UE_ROOT` to the UE_5.7 directory before running the launcher.

## Controls

| Input | Action |
|---|---|
| W / A / S / D | Walk |
| Mouse | Look |
| Space | Jump |
| E | Use the wall light switch when targeted |
| L | Toggle all main lights from anywhere for comparison/debugging |
| 1 | Toggle front teaching-zone lights |
| 2 | Toggle central table-zone lights |
| 3 | Toggle rear table-zone lights |
| 4 | Toggle stage-zone lights |
| Esc | Exit the standalone walkthrough |

## Repository layout

```text
HumanityTrinityRebuild.blend
HumanityTrinityRebuild.glb
HumanityTrinityRebuild_BlenderGenerator.py
HumanityTrinityRebuild_建模说明.md
HumanityTrinityRebuild_Unreal交互原型_使用说明.md
Tools/
  Blender/export_humanity_trinity_rebuild.py
  Unreal/build_and_setup_humanity_trinity_rebuild.ps1
  Unreal/setup_humanity_trinity_rebuild_unreal.py
UnrealProject/
  HumanityTrinityRebuild.uproject
  Config/
  Content/
  Source/HumanityTrinityRebuild/
```

Unreal-generated `Binaries`, `Intermediate`, `DerivedDataCache`, `Saved`, IDE state, logs, and Blender recovery files are intentionally excluded from Git.

## Regenerate the Blender model

The dimensions and layout parameters are grouped in the `P` dictionary near the top of `HumanityTrinityRebuild_BlenderGenerator.py`.

```powershell
& 'D:\Blender\blender-4.5.13-windows-x64\blender.exe' `
  --background `
  --python '.\HumanityTrinityRebuild_BlenderGenerator.py'
```

This regenerates the `.blend`, full-scene `.glb`, top view, and two perspective images.

## Export the Unreal runtime mesh

```powershell
& 'D:\Blender\blender-4.5.13-windows-x64\blender.exe' `
  --background '.\HumanityTrinityRebuild.blend' `
  --python '.\Tools\Blender\export_humanity_trinity_rebuild.py'
```

The exporter excludes Blender-only lights, cameras, and annotations, combines the runtime geometry, verifies the room bounds, and writes:

```text
UnrealProject/Content/SourceAssets/HumanityTrinityRebuildEnvironment_Runtime.glb
```

## Rebuild and set up the Unreal project

```powershell
powershell -ExecutionPolicy Bypass `
  -File '.\Tools\Unreal\build_and_setup_humanity_trinity_rebuild.ps1'
```

The setup script compiles the renamed native module, imports the runtime GLB, applies complex-as-simple collision, and creates or updates:

```text
/Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildWalkthrough
```

## Lighting and adaptation defaults

- Main lights: 9 rectangular lights, 2600 lm each.
- Door-leak residual light: 12 lm, directional and narrow.
- Display standby residual light: 4 lm, directional.
- Light-adapted exposure: -3.2 EV.
- Dark-adapted target: +1.25 EV.
- Dark adaptation interpolation speed: 0.16.
- Bright adaptation interpolation speed: 2.7.

The residual sources are experiential assumptions, not confirmed site fixtures. Setting both residual intensities to zero produces a physically black sealed room; exposure adaptation alone does not create light.

## Additional views

![Top view](HumanityTrinityRebuild_TopView.png)

![Perspective toward the teaching wall](HumanityTrinityRebuild_PerspectiveToTeaching.png)
