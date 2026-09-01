#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HumanityTrinityRebuildHUD.generated.h"

UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};
