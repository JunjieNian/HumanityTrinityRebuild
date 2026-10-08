#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanityTrinityRebuildTeacherPatrol.generated.h"

class AHumanityTrinityRebuildCorridor;
class AHumanityTrinityRebuildTeacher;
class AHumanityTrinityRebuildHider;
class AHumanityTrinityRebuildRoomInteraction;
class AHumanityTrinityRebuildLightingController;
class USoundWave;
class USoundAttenuation;

UENUM()
enum class ETeacherPatrolPhase : uint8
{
    Dormant, Warning, Opening, Inspecting, Departing, Recovering, Failed
};

// One opt-in director per game world; the session choice survives mode travel.
UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildTeacherPatrol : public AActor
{
    GENERATED_BODY()
public:
    AHumanityTrinityRebuildTeacherPatrol();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    static AHumanityTrinityRebuildTeacherPatrol* Find(UWorld* World);
    static void SetSessionEnabled(bool bInEnabled);
    static bool IsSessionEnabled();
    void SetEnabled(bool bInEnabled);
    bool IsEnabled() const { return bEnabled; }
    bool IsActive() const { return Phase != ETeacherPatrolPhase::Dormant; }
    bool HasFailed() const { return Phase == ETeacherPatrolPhase::Failed; }
    bool StartPatrol();
    void CancelPatrol();
    ETeacherPatrolPhase GetPhase() const { return Phase; }
    FString GetStatusLine() const;
    FString GetFailureReason() const { return FailureReason; }
    float GetSecondsRemaining() const;
    bool IsActorSafelyHidden(const AActor* Actor) const;
    int32 GetCompletedPatrolCount() const { return CompletedPatrols; }
    bool HasAllAudio() const;

    UPROPERTY(EditAnywhere, Category="Teacher patrol") float WarningSeconds = 24.f;
    UPROPERTY(EditAnywhere, Category="Teacher patrol") float InspectionSeconds = 6.f;
    UPROPERTY(EditAnywhere, Category="Teacher patrol") float FailureExitSeconds = 8.f;
    UPROPERTY(EditAnywhere, Category="Teacher patrol") float FirstDelayMin = 35.f;
    UPROPERTY(EditAnywhere, Category="Teacher patrol") float FirstDelayMax = 60.f;
    UPROPERTY(EditAnywhere, Category="Teacher patrol") float RepeatDelayMin = 45.f;
    UPROPERTY(EditAnywhere, Category="Teacher patrol") float RepeatDelayMax = 90.f;
    // Only an automated test should suppress the requested failure exit.
    bool bSuppressFailureExit = false;

private:
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildCorridor> Corridor;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildTeacher> Teacher;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildRoomInteraction> Room;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildLightingController> Lighting;
    UPROPERTY() TArray<TObjectPtr<AHumanityTrinityRebuildHider>> Participants;
    UPROPERTY() TObjectPtr<USoundWave> Footsteps;
    UPROPERTY() TObjectPtr<USoundWave> Keys;
    UPROPERTY() TObjectPtr<USoundWave> WarningVoice;
    UPROPERTY() TObjectPtr<USoundWave> ClearVoice;
    UPROPERTY() TObjectPtr<USoundWave> CaughtVoice;
    UPROPERTY() TObjectPtr<USoundAttenuation> AudioAttenuation;
    ETeacherPatrolPhase Phase = ETeacherPatrolPhase::Dormant;
    bool bEnabled = false, bExitRequested = false, bWaitForPlayerReturn = false;
    float PhaseStartedAt = 0, PhaseEndsAt = 0, NextPatrolAt = 0, NextFootstepAt = 0, NextKeysAt = 0;
    int32 CompletedPatrols = 0;
    FString FailureReason;
    FVector TeacherPhaseOrigin = FVector::ZeroVector;
    void RefreshWorldActors();
    void ChangePhase(ETeacherPatrolPhase NewPhase, float Duration);
    void PlayTeacherSound(USoundWave* Sound, float Volume = 1.f);
    void Fail(const FString& Reason);
    void CheckForDiscovery();
    void FinishPatrol();
};
