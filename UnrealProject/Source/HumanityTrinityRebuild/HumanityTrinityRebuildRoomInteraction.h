#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanityTrinityRebuildRoomInteraction.generated.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class URectLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

// Movable room elements are deliberately separate from the imported building:
// their collision, visual state and illumination all follow the same controls.
UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildRoomInteraction : public AActor
{
    GENERATED_BODY()

public:
    AHumanityTrinityRebuildRoomInteraction();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "HumanityTrinityRebuild|Interaction")
    void ToggleCurtains();
    UFUNCTION(BlueprintCallable, Category = "HumanityTrinityRebuild|Interaction")
    void SetCurtainsOpen(bool bOpen);
    UFUNCTION(BlueprintCallable, Category = "HumanityTrinityRebuild|Interaction")
    void ToggleScreen();
    UFUNCTION(BlueprintCallable, Category = "HumanityTrinityRebuild|Interaction")
    void SetScreenOn(bool bOn);

    bool AreCurtainsOpen() const { return bCurtainsTargetOpen; }
    bool IsScreenOn() const { return bScreenOn; }
    float GetCurtainOpenFraction() const { return CurtainOpenFraction; }
    bool HasCurtainMesh() const { return bHasCurtainMesh; }
    bool IsScreenIlluminating() const;
    bool IsCurtainComponent(const UPrimitiveComponent* Component) const;
    bool IsScreenComponent(const UPrimitiveComponent* Component) const;
    FString GetInteractionPrompt(const UPrimitiveComponent* Component) const;
    void Interact(UPrimitiveComponent* Component);

    UPROPERTY(EditAnywhere, Category = "HumanityTrinityRebuild|Curtains", meta = (ClampMin = "0.2"))
    float CurtainTravelSeconds = 2.4f;

private:
    UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftCurtain;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RightCurtain;
    UPROPERTY() TObjectPtr<UBoxComponent> LeftCurtainBounds;
    UPROPERTY() TObjectPtr<UBoxComponent> RightCurtainBounds;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ScreenFace;
    UPROPERTY() TObjectPtr<UBoxComponent> ScreenControlBounds;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ScreenControl;
    UPROPERTY() TObjectPtr<UTextRenderComponent> ScreenHeading;
    UPROPERTY() TObjectPtr<UTextRenderComponent> ScreenCaption;
    UPROPERTY() TObjectPtr<URectLightComponent> ScreenLight;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ScreenMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ControlMaterial;

    bool bCurtainsTargetOpen = true;
    bool bScreenOn = false;
    bool bHasCurtainMesh = false;
    float CurtainOpenFraction = 1.0f;
    void UpdateCurtainGeometry();
};
