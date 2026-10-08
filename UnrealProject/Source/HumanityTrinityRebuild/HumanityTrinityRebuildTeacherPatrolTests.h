#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanityTrinityRebuildTeacherPatrolTests.generated.h"

class AHumanityTrinityRebuildTeacherPatrol;
class AHumanityTrinityRebuildPlayerCharacter;
class AHumanityTrinityRebuildRoomInteraction;
class AHumanityTrinityRebuildLightingController;
class AHumanityTrinityRebuildCorridor;
class AHumanityTrinityRebuildHider;
class ACameraActor;

// Only spawned for an explicit developer self-test command-line switch.
UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildTeacherPatrolTests : public AActor
{
    GENERATED_BODY()
public:
    AHumanityTrinityRebuildTeacherPatrolTests();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
private:
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildTeacherPatrol> Patrol;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildPlayerCharacter> Player;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildRoomInteraction> Room;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildLightingController> Lighting;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildCorridor> Corridor;
    UPROPERTY() TObjectPtr<AHumanityTrinityRebuildHider> NPC;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    FString Scenario = TEXT("safe");
    float Elapsed = 0, PhaseElapsed = 0, InitialRoundTime = 0, NPCDistance = 0;
    FVector PreviousNPC = FVector::ZeroVector;
    int32 Step = 0;
    bool bPassed = true, bCapturedInspection = false, bFinished = false;
    void Check(const TCHAR* Name, bool bOK);
    void PlaceInRoom(int32 DoorIndex);
    void Capture(const TCHAR* Name);
    void Finish();
};
