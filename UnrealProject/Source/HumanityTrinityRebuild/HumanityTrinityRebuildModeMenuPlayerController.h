#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HumanityTrinityRebuildModeMenuPlayerController.generated.h"

UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildModeMenuPlayerController : public APlayerController
{
    GENERATED_BODY()

  protected:
    virtual void SetupInputComponent() override;

  private:
    void ChooseWalkthrough();
    void ChooseHideAndSeek();
    void ClickMenu();
    void ReturnToModeMenu();
};
