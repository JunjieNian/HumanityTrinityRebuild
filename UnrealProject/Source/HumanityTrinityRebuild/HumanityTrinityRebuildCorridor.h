#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanityTrinityRebuildCorridor.generated.h"

class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class URectLightComponent;

/** A permanent lit corridor, and three opaque, closed-by-default classroom doors. */
UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildCorridor : public AActor
{
    GENERATED_BODY()
public:
    AHumanityTrinityRebuildCorridor();
    virtual void Tick(float DeltaSeconds) override;
    static AHumanityTrinityRebuildCorridor* Find(UWorld* World);
    void SetDoorOpen(int32 Index, bool bOpen);
    float GetDoorOpenFraction(int32 Index) const;
    void SetPatrolDoorLocked(bool bLocked) { bPatrolDoorLocked = bLocked; }
    bool IsPatrolDoorLocked() const { return bPatrolDoorLocked; }
    int32 DoorIndexForComponent(const UPrimitiveComponent* Component) const;
    bool ToggleDoor(int32 Index);
    FString GetDoorPrompt(int32 Index) const;
    FVector GetDoorwayLocation(int32 Index) const;
    FVector GetCorridorApproachLocation() const;
    FVector GetInspectionLocation() const;
    int32 GetLitFixtureCount() const { return CorridorLights.Num(); }
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CorridorParts;
    UPROPERTY() TArray<TObjectPtr<URectLightComponent>> CorridorLights;
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> DoorPivots;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> DoorLeaves;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> DoorHardware;
    TArray<float> DoorFractions;
    TArray<bool> DoorTargets;
    bool bPatrolDoorLocked = false;
};
