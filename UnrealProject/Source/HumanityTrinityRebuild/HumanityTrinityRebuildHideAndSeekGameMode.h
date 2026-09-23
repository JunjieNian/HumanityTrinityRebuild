#pragma once

#include "CoreMinimal.h"
#include "HumanityTrinityRebuildGameMode.h"
#include "HumanityTrinityRebuildHideAndSeekGameMode.generated.h"

class AHumanityTrinityRebuildHider;
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
    void ReportSeekerNoise(const FVector& Location, float Loudness);
    FString GetPracticeHint() const;

  protected:
    virtual void BeginPlay() override;

  private:
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildHider> Hider;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildPlayerCharacter> Seeker;
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
};
