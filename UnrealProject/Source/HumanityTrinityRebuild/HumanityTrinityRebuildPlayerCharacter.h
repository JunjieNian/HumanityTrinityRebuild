#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HumanityTrinityRebuildLightingController.h"
#include "HumanityTrinityRebuildPlayerCharacter.generated.h"

class AHumanityTrinityRebuildLightSwitch;
class AHumanityTrinityRebuildLightingController;
class UCameraComponent;

UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AHumanityTrinityRebuildPlayerCharacter();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintPure, Category = "HumanityTrinityRebuild|Vision")
    float GetCurrentExposure() const { return CurrentExposure; }

    UFUNCTION(BlueprintPure, Category = "HumanityTrinityRebuild|Vision")
    float GetLightAdaptedExposure() const { return LightAdaptedExposure; }

    UFUNCTION(BlueprintPure, Category = "HumanityTrinityRebuild|Interaction")
    FString GetCurrentInteractionPrompt() const;

private:
    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY()
    TObjectPtr<AHumanityTrinityRebuildLightingController> LightingController;

    UPROPERTY()
    TObjectPtr<AHumanityTrinityRebuildLightSwitch> FocusedSwitch;

    UPROPERTY(EditAnywhere, Category = "HumanityTrinityRebuild|Interaction")
    float InteractionDistanceCm = 240.0f;

    UPROPERTY(EditAnywhere, Category = "HumanityTrinityRebuild|Vision")
    float LightAdaptedExposure = -3.2f;

    UPROPERTY(EditAnywhere, Category = "HumanityTrinityRebuild|Vision")
    float DarkAdaptedExposure = 1.25f;

    UPROPERTY(EditAnywhere, Category = "HumanityTrinityRebuild|Vision")
    float DarkAdaptationSpeed = 0.16f;

    UPROPERTY(EditAnywhere, Category = "HumanityTrinityRebuild|Vision")
    float BrightAdaptationSpeed = 2.7f;

    float CurrentExposure = 0.0f;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void StartJump();
    void StopJump();
    void Interact();
    void ToggleMasterLights();
    void ToggleFrontZone();
    void ToggleMiddleZone();
    void ToggleRearZone();
    void ToggleStageZone();
    void QuitPrototype();

    void UpdateFocusedInteractable();
    void UpdateEyeAdaptation(float DeltaSeconds);
    AHumanityTrinityRebuildLightingController* FindLightingController();
};
