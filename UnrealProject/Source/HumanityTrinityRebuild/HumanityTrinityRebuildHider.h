#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanityTrinityRebuildHider.generated.h"
class UCapsuleComponent;
class UStaticMeshComponent;
class USoundWave;
class USoundAttenuation;
enum class EHiderState : uint8
{
    Hidden,
    Listening,
    Relocating,
    Caught
};
UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildHider : public AActor
{
    GENERATED_BODY()
  public:
    AHumanityTrinityRebuildHider();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void SetSeeker(AActor* InSeeker);
    void HearNoise(const FVector& Location, float Loudness);
    bool HasFootstepAudio() const { return FootstepSound != nullptr; }
    int32 GetStepsEmitted() const { return StepsEmitted; }
    int32 GetCoverCount() const { return Covers.Num(); }
    int32 GetHeardCount() const { return HeardCount; }
    int32 GetRouteLength() const { return Route.Num(); }
    int32 GetLoadedPartCount() const;
    bool IsCrouching() const { return CrouchAlpha > 0.9f; }
    EHiderState GetHideState() const { return State; }
    FString GetBehaviorLabel() const;
    void SetShowcasePose(int32 Pose);

  protected:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Body;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    UPROPERTY() TObjectPtr<USoundWave> FootstepSound;
    UPROPERTY() TObjectPtr<USoundAttenuation> FootstepAttenuation;
    UPROPERTY() TObjectPtr<AActor> Seeker;
    struct FRouteNode
    {
        FVector Position;
        TArray<int32> Neighbors;
        float Cover = 0;
    };
    TArray<FRouteNode> Nodes;
    TArray<int32> Covers, Route, RecentCovers;
    EHiderState State = EHiderState::Hidden;
    FVector LastHeard = FVector::ZeroVector;
    float MemoryUntil = 0, ReactionAt = 0, ReplanAt = 0, DistanceSinceStep = 0;
    float PoseTime = 0, CrouchAlpha = 0, WalkAlpha = 0, BlockedSeconds = 0;
    int32 RouteCursor = 0, StepsEmitted = 0, HeardCount = 0, ShowcasePose = -1;
    bool bHasNoiseMemory = false;
    int32 PlannedNoiseCount = 0;
    void BuildRoutes(bool bWholeClassroom = false);
    int32 NearestNode(const FVector& Position) const;
    bool IsPassageClear(const FVector& A, const FVector& B) const;
    bool ChooseEscape();
    void SetState(EHiderState NewState);
    void Animate(float DeltaSeconds, float Distance);
    void UpdateBodyAndFootsteps(float DeltaSeconds, const FVector& PreviousLocation);
    virtual bool WantsCrouch() const;
    virtual float GetReachPose() const { return 0.f; }
};
