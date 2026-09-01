# HumanityTrinityRebuild Unreal 交互原型

## 当前实现

- Unreal Engine 5.7.4 原生 C++ 项目。
- 完整导入 Blender 三一模型，保留 20 种材质。
- 模型使用复杂碰撞，可阻止人物穿过墙体、舞台、书架和桌椅。
- 第一人称行走与鼠标观察。
- 前门门框旁实墙上的暂定总开关，使用视线射线交互。
- 屏幕中央准星、开关操作提示和底部快捷键提示。
- 九盏动态主灯，分为前方教学区、中央桌区、后排桌区和舞台区。
- 关灯后主灯功率真正归零。
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
| E | 当准星指向墙面开关时，开/关教室总灯 |
| L | 无需靠近开关，直接切换总灯，供调试 |
| 1 | 切换前方教学区灯 |
| 2 | 切换中央桌区灯 |
| 3 | 切换后排桌区灯 |
| 4 | 切换舞台区灯 |
| Esc | 退出独立漫游窗口 |

## 灯光体验

主灯关闭时：

1. 九盏主灯全部关闭；
2. 前门附近保留约 12 流明的窄条暖色门缝光；
3. 大屏附近保留约 4 流明的蓝色待机面光；
4. 观察者刚关灯时几乎全黑；
5. 曝光缓慢增加，约十几秒后只能勉强辨认近处家具轮廓，教室深处仍接近全黑。

重新开灯时，曝光会更快回到正常值，以模拟从暗处突然见光的短暂刺眼。

## 关键参数

灯具位置、分区和光强位于：

UnrealProject\Source\HumanityTrinityRebuild\HumanityTrinityRebuildLightingController.cpp

默认参数：

- 主灯：2600 lm / 盏；
- 前门门缝光：12.0 lm；
- 屏幕待机光：4.0 lm。

明暗适应位于：

UnrealProject\Source\HumanityTrinityRebuild\HumanityTrinityRebuildPlayerCharacter.h

默认参数：

- 正常开灯曝光：-3.2 EV；
- 暗适应目标曝光：+1.25 EV（相对增加 4.45 EV）；
- 暗适应速度：0.16；
- 亮适应速度：2.7。

墙面开关暂定位置位于：

UnrealProject\Source\HumanityTrinityRebuild\HumanityTrinityRebuildGameMode.cpp

## 已完成的自动验收

最终版本已在 Unreal Engine 5.7.4 中重新编译、重新导入模型，并以实际游戏关卡运行自检：

- 开灯：9 盏主灯开启，2 盏残余光关闭；
- 关灯：9 盏主灯关闭，2 盏残余光开启；
- 8 秒暗适应：曝光由 -3.200 EV 变化至 +0.014 EV；
- 重新开灯 2 秒：曝光恢复至 -3.185 EV；
- 玩家视角射线在 196.3 cm 处命中墙面开关；
- 同一个开关可实际关闭全部主灯，再次操作可恢复；
- 自检最终结果：PASS。

核对截图位于：

UnrealProject\Saved\Screenshots\HumanityTrinityRebuild

其中包含开灯、刚关灯、暗适应后、重新开灯和墙面开关近景五张图。

## 模型修改后的更新流程

1. 在 Blender 中修改并保存 HumanityTrinityRebuild.blend。
2. 运行 Unreal 专用导出：

   D:\Blender\blender-4.5.13-windows-x64\blender.exe --background D:\HumanityTrinityRebuild\HumanityTrinityRebuild.blend --python D:\HumanityTrinityRebuild\Tools\Blender\export_humanity_trinity_rebuild.py

3. 运行：

   powershell -ExecutionPolicy Bypass -File D:\HumanityTrinityRebuild\Tools\Unreal\build_and_setup_humanity_trinity_rebuild.ps1

脚本会重新编译 C++、导入模型、更新复杂碰撞并保存关卡。

## 当前仍为暂定的内容

- 墙面开关的准确位置和样式；
- 灯具实际数量、型号、色温、功率和分组；
- 前门漏光和大屏待机灯是否真实存在；
- 人眼适应参数；
- 人物起始位置；
- 真实门扇开启方向和能否交互。

这些参数均与已确认的空间几何分离，可以在不重建教室的情况下继续调整。

当前门缝光和屏幕待机光是为了建立“近乎全黑但仍可体验暗适应”的模拟条件，并不是已经由现场资料确认存在的灯源。如果以后确认真实教室关灯后完全没有任何残余光，可将两个残余光强都改为 0；此时暗适应只会改变相机曝光，不会凭空产生可见物体。
