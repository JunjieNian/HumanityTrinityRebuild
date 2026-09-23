# HumanityTrinityRebuild Unreal 交互原型

## 黑暗捉迷藏雏形

双击 `Launch_HumanityTrinityRebuild_HideAndSeek.cmd` 进入独立游戏关卡。前 12 秒开灯记住教室，之后主灯、门缝残光和屏幕待机光关闭；你有三分钟用空间记忆、脚步声和近距离触摸找出一名电脑控制的藏者。藏者会安静地停留，也会沿桌组间的通道移动，移动时发出有方向的脚步声。建议佩戴耳机。

- `W/A/S/D` 移动、鼠标转向、`Shift` 慢行。
- 按住 `F` 向前伸手连续摸索，移动会减慢；只反馈手边约一米内的接触。摸到藏者才算抓获。
- `E` 只操作手边的舞台暗门；灯光、幕布和屏幕的快捷键在游戏中锁定。
- `R` 重开一局；`Esc` 退出。

目前是单人玩法雏形：藏者的路线较简单，触摸物体的分类还较粗略，尚未实现双人联机、手部动画或手柄震动。原来的漫游关卡和入口仍然独立存在。游戏开发前的完整可编辑建模版本已固定为远端标签 `v1.0.0-reconstruction`。

## 当前实现

- Unreal Engine 5.7.4 原生 C++ 项目。
- 导入经过照片细化的 Blender 模型，包括木纹舞台、灰绿地坪、酒红幕布、白色高柜、蓝黄白桌面和脚轮家具，并保留可移植材质贴图。
- 模型使用复杂碰撞，可阻止人物穿过墙体、舞台、书架和桌椅。
- 第一人称行走、鼠标观察，以及便于近看材质和物件的慢行模式。
- 前门门框旁实墙上的暂定总开关，使用视线射线交互。
- 屏幕中央准星、近距离交互提示和底部快捷键提示。
- 幕布可缓动开合；关闭时遮挡舞台并阻挡人物直接通过。
- 教学屏可独立开关，带局部光照；默认关闭，以便看到两扇深绿书写板。
- 二十个可见矩形灯板配合九盏动态主光源，分为前方教学区、中央桌区、后排桌区和舞台区。
- 关闭某个分区时，该区光源与可见灯板发光同步归零；四个分区全灭时也会进入暗适应。
- 可关闭的、带方向性的微弱门缝光和大屏待机面光。
- 暗适应慢、亮适应快的曝光变化。
- 无模型时仍可生成封闭的 12 × 18 × 3.4 米备用房间，便于调试。

## 直接开始漫游

双击项目根目录中的 Launch_HumanityTrinityRebuild_Walkthrough.cmd。

第一次启动可能需要等待 Unreal 编译或准备着色器。

## 在 Unreal 编辑器中打开

双击 Open_HumanityTrinityRebuild_UnrealProject.cmd。

打开后默认关卡为 /Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildWalkthrough。点击编辑器顶部的 Play 即可进入漫游。

## 操作

| 按键 | 功能 |
|---|---|
| W / A / S / D | 行走 |
| 鼠标 | 转动视线 |
| Space | 跳跃 |
| 按住 Shift | 慢行，便于靠近观察 |
| E | 对准附近墙面开关、幕布、讲台控制器或舞台两侧木墙暗门，操作对应对象 |
| L | 无需靠近开关，直接切换总灯，供调试 |
| C | 缓动打开／关闭幕布 |
| P | 打开／关闭教学屏 |
| 1 | 切换前方教学区灯 |
| 2 | 切换中央桌区灯 |
| 3 | 切换后排桌区灯 |
| 4 | 切换舞台区灯 |
| Esc | 退出独立漫游窗口 |

## 灯光体验

默认进入时主灯开启、幕布两侧打开、教学屏关闭。可先走到前门旁的总开关，或直接按 L，比较灯光变化；走到舞台附近按 C 可观察幕布开合，再按 P 单独开启教学屏，观察它在暗处提供的局部照明。

主灯和教学屏均关闭时：

1. 九盏主灯全部关闭；
2. 前门附近保留约 12 流明的窄条暖色门缝光；
3. 大屏附近仅保留暂定的极弱待机光；
4. 观察者刚关灯时几乎全黑；
5. 曝光缓慢增加，暗适应后仅能勉强辨认近处家具轮廓，教室深处仍接近全黑。

重新开灯时，曝光会更快回到正常值，以模拟从暗处突然见光的短暂刺眼。

教学屏开启时会发光，并在附近形成弱的冷白照明，因此不会呈现全室近黑的同一状态。相机有专门的屏幕照明曝光目标，避免照搬“所有光源都灭”的曝光导致屏幕过亮。检查纯粹关灯效果时，应同时关闭教学屏。

二十个灯板与九个实际光源数量不同，是当前性能和外观之间的实现选择；灯板数量、分区、亮度、色温、残光和人眼适应参数均是可调整的近似值，不是根据照片测出的现场数值。

## 幕布、教学屏与慢行

幕布在约 2.4 秒内缓动打开或关闭。C 可直接控制；靠近并对准幕布按 E 也可操作。关闭的布面遮挡可见射线和人物通路，开启后恢复舞台通路。这里提供的是交互演示，真实幕布是否电动、如何拉动仍待确认。

P 直接开关教学屏；走到讲台附近，对准顶部深灰色小控制器按 E 可进行同样操作。面向教学墙，开启后右侧黑板连同边框沿前轨道左移，约 2 秒后与左侧黑板重叠，露出固定在右侧后方的显示屏并亮屏。关闭时先熄屏，再将右板滑回原位。滑动中再次按 P 或操作控制器会从当前位置平滑反向，不会跳回起点；下一位置与角色重叠时会暂停。黑板的前后轨道分层，重叠时不会相互穿插。

屏幕文字、发光面和局部照明均在黑板完全移开后启用，关闭时立即熄灭；待机微光也位于右侧黑板后方并受其遮挡。实际机械关系来自用户现场补充，动画时间与自动联动不代表已确认现场有电动机构。

默认行走速度为 240 cm/s，按住 Shift 后为 120 cm/s。人物尺度和速度用于原型体验，仍可根据实际空间感继续调整。

舞台左右斜木墙上各有一扇齐平暗门，以细门缝区分，表面材质与木墙一致。靠近并对准按 E 打开后可进入道具间，再次对准门扇按 E 关闭。门扇会在碰到人物前暂停转动。门洞是实际几何开口；内侧暂设 21 cm 过渡踏步，开启方向与尺寸可继续调整。

相邻桌组的椅子已向桌边收进，并对包括椅背、支架和脚轮在内的完整轮廓进行间距检查。当前 54 把小组桌椅之间的保守最小间距约 22 cm。

站到 76 cm 高的桌面时，视点约在地面以上 234 cm，距离 340 cm 的天花板约 106 cm。顶面增加了与各灯区同步的反射补光，解决仰视时依赖屏幕内反射而变黑的问题；关灯时补光同样归零。补光是间接照明的近似，不表示现场多出了灯具。

## 关键参数

灯具位置、分区和光强位于：

UnrealProject\Source\HumanityTrinityRebuild\HumanityTrinityRebuildLightingController.cpp

默认参数：

- 教室主灯：3400 lm / 4500 K，舞台主灯：5200 lm / 4000 K；
- 前门门缝光：12.0 lm；
- 设备待机残光：0.6 lm。

明暗适应位于：

UnrealProject\Source\HumanityTrinityRebuild\HumanityTrinityRebuildPlayerCharacter.h

默认参数：

- 正常开灯曝光：-4.7 EV；
- 暗适应目标曝光：+1.25 EV（相对增加 5.95 EV）；
- 暗适应速度：0.16；
- 亮适应速度：2.7。

墙面开关暂定位置位于：

UnrealProject\Source\HumanityTrinityRebuild\HumanityTrinityRebuildGameMode.cpp

## 自检与截图核对

项目提供在实际游戏关卡中运行的自动自检入口，当前核对范围包括：

- 总灯和各分区的有效光源状态；
- 灯板发光与对应分区同步关闭；
- 四分区全灭后的暗适应及重新开灯后的恢复；
- 玩家视角射线命中墙面开关并真正完成开关灯；
- 幕布开合后的状态、遮挡与交互；
- 教学屏开关及其对应的曝光条件；右板左移后的重叠位置、右侧屏幕九点可见性、合拢后九点遮挡、中途反向及关闭归位。

运行示例：

```powershell
& 'D:\EpicGames\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  '.\UnrealProject\HumanityTrinityRebuild.uproject' `
  '/Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildWalkthrough' `
  -game -RenderOffscreen -ResX=1280 -ResY=720 -unattended -nosound `
  -HumanityTrinityRebuildSelfTest -HumanityTrinityRebuildCapture -log
```

结果应查看当次运行日志 `UnrealProject\Saved\Logs\HumanityTrinityRebuild.log` 中的 `[HUMANITY_TRINITY_REBUILD_SELFTEST]` 记录；旧版本通过记录不代表修改后的版本也已通过。

核对截图位于：

UnrealProject\Saved\Screenshots\HumanityTrinityRebuild

其中包括开灯、刚关灯、暗适应后、重新开灯、墙面开关近景，以及 `05_StageOpen`、`06_StageClosed`、`07_TeachingView` 等新增状态图。用于交付核对的精选截图放在 `Docs\Previews`；临时日志和完整 Saved 目录不纳入版本库。

## 模型修改后的更新流程

1. 修改 HumanityTrinityRebuild_BlenderGenerator.py 的参数或 Tools/Blender/photo_details.py 的细节，再运行生成脚本；也可直接在 Blender 中修改并保存 HumanityTrinityRebuild.blend，但下次重新生成时会覆盖手工改动。
2. 运行 Unreal 专用导出：

   D:\Blender\blender-4.5.13-windows-x64\blender.exe --background D:\HumanityTrinityRebuild\HumanityTrinityRebuild.blend --python D:\HumanityTrinityRebuild\Tools\Blender\export_humanity_trinity_rebuild.py

3. 运行：

   powershell -ExecutionPolicy Bypass -File D:\HumanityTrinityRebuild\Tools\Unreal\build_and_setup_humanity_trinity_rebuild.ps1

脚本会重新编译 C++、导入模型、更新复杂碰撞并保存关卡。

可动幕布、教学屏、实体控制器和可切换灯板由 Unreal 运行时逻辑管理。只修改 C++ 交互逻辑时，需要重新编译并启动体验；修改静态场景或贴图后，需要重新导出与导入。两类内容都修改时按上述完整流程更新。

## 当前仍为暂定的内容

- 墙面开关的准确位置和样式；
- 灯具实际数量、型号、色温、功率和分组；
- 前门漏光和大屏待机灯是否真实存在；
- 人眼适应参数；
- 人物起始位置；
- 三扇外侧门的真实开启方向；两扇舞台暗门的铰链位置、开启方向和内侧台阶尺寸。
- 幕布实际操作方式、教学屏与黑板的准确尺寸、轨道间距及实际操作装置（右板左移露出右侧屏幕已确认）；
- 桌椅总数与位置、台阶和吧台的实测尺寸。

这些参数均与已确认的空间几何分离，可以在不重建教室的情况下继续调整。

当前门缝光和屏幕待机光是为了建立“近乎全黑但仍可体验暗适应”的模拟条件，并不是已经由现场资料确认存在的灯源。如果以后确认真实教室关灯后完全没有任何残余光，可将两个残余光强都改为 0；此时暗适应只会改变相机曝光，不会凭空产生可见物体。
