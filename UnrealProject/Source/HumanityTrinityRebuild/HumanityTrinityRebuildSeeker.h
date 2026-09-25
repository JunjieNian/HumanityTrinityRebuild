#pragma once

#include "CoreMinimal.h"
#include "HumanityTrinityRebuildHider.h"
#include "HumanityTrinityRebuildSeeker.generated.h"

enum class ETrinitySearchState : uint8
{
    Waiting,
    Patrolling,
    Listening,
    Investigating,
    Feeling,
    Finished
};

// Shares the articulated body and collision-tested floor graph with the hider.
// Player position is used only by physical contact queries, never to choose a route.
UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildSeeker : public AHumanityTrinityRebuildHider
{
    GENERATED_BODY()

  public:
    void PrepareToSeek(AActor* Player);
    void StartSearching();
    void StopSearching();
    void HearPlayerNoise(const FVector& Location, float Loudness);
    virtual void Tick(float DeltaSeconds) override;
    FString GetSearchLabel() const;
    ETrinitySearchState GetSearchState() const { return SearchState; }

  protected:
    virtual bool WantsCrouch() const override;
    virtual float GetReachPose() const override;

  private:
    friend class AHumanityTrinityRebuildHideAndSeekGameMode;
    ETrinitySearchState SearchState = ETrinitySearchState::Waiting;
    TArray<float> LastVisited;
    FVector SearchOrigin = FVector::ZeroVector;
    float PauseUntil = 0, NextTouchAt = 0, NextReactionAt = 0, AvoidUntil = 0;
    float FeelingStartedAt = 0, DistanceTravelled = 0;
    int32 LocalSearchesLeft = 0, StopsSearched = 0, AvoidNode = INDEX_NONE;
    bool bSearching = false;

    void SetSearchState(ETrinitySearchState NewState);
    bool PlanTo(int32 Goal);
    void ChoosePatrolStop();
    void InvestigateSound();
    void BeginFeeling();
    void FinishFeeling();
    bool FeelAhead();
};
