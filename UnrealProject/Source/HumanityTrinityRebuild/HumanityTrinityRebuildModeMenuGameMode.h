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
    virtual bool IsTeacherPatrolAllowed() const override { return !bChoosingMode; }
    void ToggleTeacherPatrol();
    bool IsTeacherPatrolEnabled() const;
    void EnterWalkthrough();
    void EnterHideAndSeek();
    void EnterPlayerHiding();
    void ReturnToMenu();

  protected:
    virtual void BeginPlay() override;

  private:
    bool bChoosingMode = true;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildPlayerCharacter> MenuPlayer;
    void InitializeMenu();
    void RunMenuSelfTest();
};
