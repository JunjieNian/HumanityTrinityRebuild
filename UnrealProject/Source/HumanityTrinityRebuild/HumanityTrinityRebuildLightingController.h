#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HumanityTrinityRebuildLightingController.generated.h"

class URectLightComponent;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EHumanityTrinityRebuildLightZone : uint8
{
    Front  UMETA(DisplayName = "Front teaching zone"),
    Middle UMETA(DisplayName = "Central table zone"),
    Rear   UMETA(DisplayName = "Rear table zone"),
    Stage  UMETA(DisplayName = "Stage zone")
};

UCLASS()
class HUMANITYTRINITYREBUILD_API AHumanityTrinityRebuildLightingController : public AActor
{
    GENERATED_BODY()

public:
    AHumanityTrinityRebuildLightingController();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "HumanityTrinityRebuild|Lighting")
    void ToggleMaster();

    UFUNCTION(BlueprintCallable, Category = "HumanityTrinityRebuild|Lighting")
    void SetMasterLights(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "HumanityTrinityRebuild|Lighting")
    void ToggleZone(EHumanityTrinityRebuildLightZone Zone);

    UFUNCTION(BlueprintCallable, Category = "HumanityTrinityRebuild|Lighting")
    void SetZoneEnabled(EHumanityTrinityRebuildLightZone Zone, bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "HumanityTrinityRebuild|Lighting")
    bool AreMainLightsOn() const { return bMasterLightsOn; }

    UFUNCTION(BlueprintPure, Category = "HumanityTrinityRebuild|Lighting")
    bool IsZoneEnabled(EHumanityTrinityRebuildLightZone Zone) const;

    UFUNCTION(BlueprintPure, Category = "HumanityTrinityRebuild|Lighting")
    int32 GetActiveMainLightCount() const;

    UFUNCTION(BlueprintPure, Category = "HumanityTrinityRebuild|Lighting")
    int32 GetActiveResidualLightCount() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HumanityTrinityRebuild|Lighting")
    bool bMasterLightsOn = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HumanityTrinityRebuild|Lighting")
    bool bResidualLightsEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HumanityTrinityRebuild|Lighting", meta = (ClampMin = "0.0"))
    float MainLightIntensityLumens = 2600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HumanityTrinityRebuild|Lighting", meta = (ClampMin = "0.0"))
    float DoorLeakIntensityLumens = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HumanityTrinityRebuild|Lighting", meta = (ClampMin = "0.0"))
    float ScreenStandbyIntensityLumens = 4.0f;

private:
    UPROPERTY(VisibleAnywhere, Category = "HumanityTrinityRebuild|Lighting")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "HumanityTrinityRebuild|Lighting")
    TArray<TObjectPtr<URectLightComponent>> MainLights;

    UPROPERTY(VisibleAnywhere, Category = "HumanityTrinityRebuild|Lighting")
    TArray<TObjectPtr<URectLightComponent>> ResidualLights;

    UPROPERTY(VisibleAnywhere, Category = "HumanityTrinityRebuild|Lighting")
    TArray<TObjectPtr<UStaticMeshComponent>> PanelVisuals;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> PanelMaterials;

    TArray<EHumanityTrinityRebuildLightZone> MainLightZones;
    TArray<EHumanityTrinityRebuildLightZone> PanelZones;

    bool bFrontZoneOn = true;
    bool bMiddleZoneOn = true;
    bool bRearZoneOn = true;
    bool bStageZoneOn = true;

    void ApplyLightingState();
    bool& GetMutableZoneState(EHumanityTrinityRebuildLightZone Zone);
};
