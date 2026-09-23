#pragma once

#include "CoreMinimal.h"
#include "HumanityTrinityRebuildGameMode.h"
#include "HumanityTrinityRebuildModeMenuGameMode.generated.h"

class AHumanityTrinityRebuildPlayerCharacter;

UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildModeMenuGameMode : public AHumanityTrinityRebuildGameMode
{
    GENERATED_BODY()

  public:
    AHumanityTrinityRebuildModeMenuGameMode();
    bool IsChoosingMode() const { return bChoosingMode; }
    void EnterWalkthrough();
    void EnterHideAndSeek();
    void ReturnToMenu();

  protected:
    virtual void BeginPlay() override;

  private:
    bool bChoosingMode = true;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildPlayerCharacter> MenuPlayer;
    void InitializeMenu();
    void RunMenuSelfTest();
};
