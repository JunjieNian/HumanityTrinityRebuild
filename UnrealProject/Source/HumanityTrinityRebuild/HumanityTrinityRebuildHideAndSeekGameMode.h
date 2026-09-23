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
    virtual void Tick(float DeltaSeconds) override;
    bool IsRoundRunning() const { return bRoundRunning; }
    bool IsRoundFinished() const { return bRoundFinished; }
    float GetSecondsRemaining() const { return SecondsRemaining; }
    FString GetStatusLine() const;
    void TryCatchHider(AActor* TouchedActor);
    void RestartRound();

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
};
