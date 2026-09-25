#pragma once

#include "CoreMinimal.h"
#include "HumanityTrinityRebuildGameMode.h"
#include "HumanityTrinityRebuildHideAndSeekGameMode.generated.h"

class AHumanityTrinityRebuildHider;
class AHumanityTrinityRebuildSeeker;
class AHumanityTrinityRebuildPlayerCharacter;

UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildHideAndSeekGameMode : public AHumanityTrinityRebuildGameMode
{
    GENERATED_BODY()

  public:
    AHumanityTrinityRebuildHideAndSeekGameMode();
    virtual void Tick(float DeltaSeconds) override;
    bool IsRoundRunning() const { return bRoundRunning; }
    bool IsRoundFinished() const { return bRoundFinished; }
    float GetSecondsRemaining() const { return SecondsRemaining; }
    FString GetStatusLine() const;
    void TryCatchHider(AActor* TouchedActor);
    void RestartRound();
    void TogglePracticeMode();
    bool IsPracticeMode() const { return bPracticeMode; }
    void ReportPlayerNoise(const FVector& Location, float Loudness);
    FString GetPracticeHint() const;
    bool IsPlayerHiding() const { return bPlayerHiding; }
    void ReadyToHide();
    void NotifyPlayerCaught(AActor* SearchingActor);

  protected:
    virtual void BeginPlay() override;

  private:
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildHider> Hider;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildPlayerCharacter> Seeker;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildSeeker> SearchingNPC;
    UPROPERTY() TObjectPtr<AActor> HidingBoundary;
    FTimerHandle PreparationTimer;
    float SecondsRemaining = 180.0f;
    float PreparationEndsAt = 0.0f;
    bool bRoundRunning = false;
    bool bRoundFinished = false;
    bool bSeekerWon = false;
    bool bSelfTest = false;
    bool bCaptureGame = false;
    bool bSelfTestCorePassed = false;
    bool bPracticeMode = false;
    bool bPlayerHiding = false;
    bool bPlayerHidingSelfTest = false;
    bool bPlayerHidingTestPassed = true;
    int32 ShowcaseStep = 0;
    float SelfTestMoveUntil = 0;
    FVector SelfTestHiderStart = FVector::ZeroVector;

    void BeginPreparation();
    void StartRound();
    void FinishRound(bool bCaught);
    void RunSelfTest();
    void FinishSelfTest();
    void ExitSelfTest();
    void CaptureMemorize();
    void CaptureBlackout();
    void CaptureFound();
    void CaptureGameScreenshot(const FString& FileName);
    void CaptureCharacterShowcase();
    void TestRoundControls();
    void CreateHidingBoundary();
    void RunPlayerHidingSelfTest();
    void TestPlayerHidingSound();
    void TestPlayerHidingContact();
    void TestPlayerHidingRoundControls();
    void RecordPlayerHidingCheck(const TCHAR* Name, bool bPassed);
};
