#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanityTrinityRebuildHider.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class USoundWave;
class USoundAttenuation;

UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildHider : public AActor
{
    GENERATED_BODY()

public:
    AHumanityTrinityRebuildHider();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void SetSeeker(AActor* InSeeker);
    bool HasFootstepAudio() const { return FootstepSound != nullptr; }
    int32 GetStepsEmitted() const { return StepsEmitted; }

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Silhouette;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Head;
    UPROPERTY() TObjectPtr<USoundWave> FootstepSound;
    UPROPERTY() TObjectPtr<USoundAttenuation> FootstepAttenuation;
    UPROPERTY() TObjectPtr<AActor> Seeker;

    float StillSeconds = 8.0f;
    float StepSeconds = 0.0f;
    bool bMoving = false;
    float TargetY = -650.0f;
    int32 StepsEmitted = 0;
};
