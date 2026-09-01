#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanityTrinityRebuildEnvironmentActor.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildEnvironmentActor : public AActor
{
    GENERATED_BODY()

public:
    AHumanityTrinityRebuildEnvironmentActor();

    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> ImportedEnvironment;

    UPROPERTY()
    TObjectPtr<UStaticMesh> FallbackCubeMesh;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UStaticMeshComponent>> FallbackParts;

    void BuildFallbackRoom();
    void AddFallbackBox(const TCHAR* Name, const FVector& Location, const FVector& Scale);
};
