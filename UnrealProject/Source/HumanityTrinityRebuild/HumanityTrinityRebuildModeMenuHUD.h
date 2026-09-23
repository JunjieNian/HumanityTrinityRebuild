#pragma once

#include "CoreMinimal.h"
#include "HumanityTrinityRebuildHUD.h"
#include "HumanityTrinityRebuildModeMenuHUD.generated.h"

UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildModeMenuHUD : public AHumanityTrinityRebuildHUD
{
    GENERATED_BODY()

  public:
    virtual void DrawHUD() override;
    bool SelectAt(float X, float Y);
    FVector2D GetChoiceCenter(int32 Choice) const;
    bool HasLayout() const { return ViewSize.X > 0 && ViewSize.Y > 0; }

  private:
    FVector2D ViewSize = FVector2D::ZeroVector;
    FBox2D GetChoiceBox(int32 Choice) const;
};
