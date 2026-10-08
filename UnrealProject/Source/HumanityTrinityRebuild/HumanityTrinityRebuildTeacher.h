#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanityTrinityRebuildTeacher.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/** Original, editable adult teacher with fifteen articulated imported mesh parts. */
UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildTeacher : public AActor
{
    GENERATED_BODY()
public:
    AHumanityTrinityRebuildTeacher();
    virtual void Tick(float DeltaSeconds) override;
    void SetWalking(bool bInWalking) { bWalking = bInWalking; }
    void FaceIntoRoom();
    int32 GetLoadedPartCount() const;
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY() TObjectPtr<USceneComponent> BodyPivot;
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> Joints;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    bool bWalking = false;
    float WalkBlend = 0.f;
    float AnimationTime = 0.f;
};
