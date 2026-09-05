#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HumanityTrinityRebuildGameMode.generated.h"

class AHumanityTrinityRebuildLightingController;
class AHumanityTrinityRebuildPlayerCharacter;
class AHumanityTrinityRebuildRoomInteraction;

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

    UPROPERTY()
    TObjectPtr<AHumanityTrinityRebuildRoomInteraction> SelfTestRoom;

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
    void SelfTestPrepareStageView();
    void SelfTestValidateCurtainsOpen();
    void SelfTestValidateCurtainsClosed();
    void SelfTestValidateCurtainsReopened();
    void SelfTestPrepareScreenControl();
    void SelfTestValidateScreenControl();
    void SelfTestCaptureTeachingView();
    void SelfTestValidateScreenOff();
    void SelfTestReportResult();
    bool SelfTestCurtainBlocksPassage() const;
    void SelfTestExit();
    void CaptureSelfTestScreenshot(const FString& FileName);
};
