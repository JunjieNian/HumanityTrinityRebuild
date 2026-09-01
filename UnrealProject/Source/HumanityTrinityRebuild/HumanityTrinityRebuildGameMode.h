#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HumanityTrinityRebuildGameMode.generated.h"

class AHumanityTrinityRebuildLightingController;
class AHumanityTrinityRebuildPlayerCharacter;

UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AHumanityTrinityRebuildGameMode();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TObjectPtr<AHumanityTrinityRebuildLightingController> SelfTestLighting;

    UPROPERTY()
    TObjectPtr<AHumanityTrinityRebuildPlayerCharacter> SelfTestPlayer;

    float SelfTestDarkExposureStart = 0.0f;
    float SelfTestDarkExposureObserved = 0.0f;
    bool bSelfTestPassed = true;
    bool bCaptureSelfTestScreenshots = false;

    void StartSelfTest();
    void SelfTestCaptureLightsOn();
    void SelfTestSwitchOff();
    void SelfTestCaptureJustOff();
    void SelfTestObserveDark();
    void SelfTestTurnOn();
    void SelfTestFinish();
    void SelfTestPrepareSwitchView();
    void SelfTestValidateSwitch();
    void SelfTestExit();
    void CaptureSelfTestScreenshot(const FString& FileName);
};
