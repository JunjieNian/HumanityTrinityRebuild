#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanityTrinityRebuildLightSwitch.generated.h"

class AHumanityTrinityRebuildLightingController;
class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildLightSwitch : public AActor
{
    GENERATED_BODY()

public:
    AHumanityTrinityRebuildLightSwitch();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "HumanityTrinityRebuild|Interaction")
    void Interact(AActor* Interactor);

    UFUNCTION(BlueprintPure, Category = "HumanityTrinityRebuild|Interaction")
    FString GetInteractionPrompt() const;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> InteractionBounds;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> BackPlate;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Paddle;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Label;

    UPROPERTY()
    TObjectPtr<AHumanityTrinityRebuildLightingController> LightingController;

    void UpdateVisualState();
};
