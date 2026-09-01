using UnrealBuildTool;
using System.Collections.Generic;

public class HumanityTrinityRebuildEditorTarget : TargetRules
{
    public HumanityTrinityRebuildEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("HumanityTrinityRebuild");
    }
}
