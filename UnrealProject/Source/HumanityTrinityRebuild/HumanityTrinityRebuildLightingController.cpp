#include "HumanityTrinityRebuildLightingController.h"
#include "HumanityTrinityRebuildTeachingLayout.h"

#include "Components/RectLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    struct FMainLightSpec
    {
        const TCHAR* Name;
        FVector Location;
        EHumanityTrinityRebuildLightZone Zone;
    };

    const FMainLightSpec MainLightSpecs[] =
    {
        { TEXT("FrontLight_Left"),   FVector(-250.0,  -200.0, 336.5), EHumanityTrinityRebuildLightZone::Front },
        { TEXT("FrontLight_Right"),  FVector( 250.0,  -200.0, 336.5), EHumanityTrinityRebuildLightZone::Front },
        { TEXT("MiddleLight_LeftA"), FVector(-250.0,  -530.0, 336.5), EHumanityTrinityRebuildLightZone::Middle },
        { TEXT("MiddleLight_RightA"),FVector( 250.0,  -530.0, 336.5), EHumanityTrinityRebuildLightZone::Middle },
        { TEXT("MiddleLight_LeftB"), FVector(-250.0,  -860.0, 336.5), EHumanityTrinityRebuildLightZone::Middle },
        { TEXT("MiddleLight_RightB"),FVector( 250.0,  -860.0, 336.5), EHumanityTrinityRebuildLightZone::Middle },
        { TEXT("RearLight_Left"),    FVector(-250.0, -1190.0, 336.5), EHumanityTrinityRebuildLightZone::Rear },
        { TEXT("RearLight_Right"),   FVector( 250.0, -1190.0, 336.5), EHumanityTrinityRebuildLightZone::Rear },
        { TEXT("StageLight"),        FVector(   0.0, -1580.0, 336.5), EHumanityTrinityRebuildLightZone::Stage }
    };

    EHumanityTrinityRebuildLightZone ZoneForPanelRow(const int32 Row)
    {
        if (Row == 0)
        {
            return EHumanityTrinityRebuildLightZone::Front;
        }
        if (Row <= 2)
        {
            return EHumanityTrinityRebuildLightZone::Middle;
        }
        if (Row == 3)
        {
            return EHumanityTrinityRebuildLightZone::Rear;
        }
        return EHumanityTrinityRebuildLightZone::Stage;
    }
}

AHumanityTrinityRebuildLightingController::AHumanityTrinityRebuildLightingController()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LightingRoot"));
    RootComponent = SceneRoot;

    for (const FMainLightSpec& Spec : MainLightSpecs)
    {
        URectLightComponent* Light = CreateDefaultSubobject<URectLightComponent>(Spec.Name);
        Light->SetupAttachment(SceneRoot);
        Light->SetRelativeLocation(Spec.Location);
        Light->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensityUnits(ELightUnits::Lumens);
        Light->SetIntensity(MainLightIntensityLumens);
        // Each economical area light represents a pair of the visible LED
        // panels; the stage light represents the wider bank over the platform.
        Light->SetSourceWidth(Spec.Zone == EHumanityTrinityRebuildLightZone::Stage ? 520.0f : 300.0f);
        Light->SetSourceHeight(60.0f);
        Light->SetAttenuationRadius(760.0f);
        Light->SetLightColor(FLinearColor::White);
        Light->SetUseTemperature(true);
        Light->SetTemperature(Spec.Zone == EHumanityTrinityRebuildLightZone::Stage
            ? StageTemperatureKelvin : ClassroomTemperatureKelvin);
        Light->SetCastShadows(true);
        MainLights.Add(Light);
        MainLightZones.Add(Spec.Zone);

        // Approximate diffuse room bounce independently of screen-space GI.
        // Looking up from a table can otherwise lose all ceiling illumination
        // when the lit floor leaves the screen. Every fill follows its circuit.
        URectLightComponent* Bounce = CreateDefaultSubobject<URectLightComponent>(
            *FString::Printf(TEXT("CeilingBounce_%s"), Spec.Name));
        Bounce->SetupAttachment(SceneRoot);
        Bounce->SetRelativeLocation(FVector(Spec.Location.X, Spec.Location.Y, 205.0f));
        Bounce->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
        Bounce->SetMobility(EComponentMobility::Movable);
        Bounce->SetIntensityUnits(ELightUnits::Lumens);
        Bounce->SetSourceWidth(300.0f);
        Bounce->SetSourceHeight(250.0f);
        Bounce->SetAttenuationRadius(650.0f);
        Bounce->SetUseTemperature(true);
        Bounce->SetCastShadows(true);
        Bounce->SetIndirectLightingIntensity(0.0f);
        CeilingBounceLights.Add(Bounce);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;

    const double PanelX[] = { -375.0, -125.0, 125.0, 375.0 };
    const double PanelY[] = { -200.0, -530.0, -860.0, -1190.0, -1580.0 };

    for (int32 Row = 0; Row < 5; ++Row)
    {
        for (int32 Column = 0; Column < 4; ++Column)
        {
            const FString Name = FString::Printf(TEXT("CeilingPanel_%d_%d"), Row + 1, Column + 1);
            const FString HousingName = FString::Printf(TEXT("CeilingPanelHousing_%d_%d"), Row + 1, Column + 1);
            UStaticMeshComponent* Housing = CreateDefaultSubobject<UStaticMeshComponent>(*HousingName);
            Housing->SetupAttachment(SceneRoot);
            Housing->SetStaticMesh(CubeMesh);
            Housing->SetRelativeLocation(FVector(PanelX[Column], PanelY[Row], 339.0));
            Housing->SetRelativeScale3D(FVector(1.24, 0.64, 0.02));
            Housing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Housing->SetCastShadow(false);
            PanelHousings.Add(Housing);

            UStaticMeshComponent* Panel = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
            Panel->SetupAttachment(SceneRoot);
            Panel->SetStaticMesh(CubeMesh);
            // Underside is 337.5 cm; the backing meets the 340 cm ceiling.
            Panel->SetRelativeLocation(FVector(PanelX[Column], PanelY[Row], 338.0));
            Panel->SetRelativeScale3D(FVector(1.20, 0.60, 0.01));
            Panel->SetMobility(EComponentMobility::Movable);
            Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Panel->SetCastShadow(false);
            // The rect lights supply indirect light. Excluding the emissive
            // diffuser from the distance-field scene avoids duplicate GI and
            // persistent glowing surfaces when a circuit is switched off.
            Panel->SetAffectDynamicIndirectLighting(false);
            Panel->SetAffectDistanceFieldLighting(false);
            PanelVisuals.Add(Panel);
            PanelZones.Add(ZoneForPanelRow(Row));
        }
    }

    // A low, narrow area light just inside the front door.  It reads as light
    // leaking under/around a door instead of as an unexplained glowing point.
    URectLightComponent* DoorLeak = CreateDefaultSubobject<URectLightComponent>(TEXT("DoorLeakResidualLight"));
    DoorLeak->SetupAttachment(SceneRoot);
    DoorLeak->SetRelativeLocation(FVector(-575.0, -230.0, 34.0));
    DoorLeak->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
    DoorLeak->SetMobility(EComponentMobility::Movable);
    DoorLeak->SetIntensityUnits(ELightUnits::Lumens);
    DoorLeak->SetIntensity(DoorLeakIntensityLumens);
    DoorLeak->SetSourceWidth(75.0f);
    DoorLeak->SetSourceHeight(7.0f);
    DoorLeak->SetAttenuationRadius(480.0f);
    DoorLeak->SetLightColor(FLinearColor(1.0f, 0.72f, 0.42f));
    DoorLeak->SetCastShadows(true);
    ResidualLights.Add(DoorLeak);

    // A tiny standby indicator on the teaching equipment, not a luminous
    // blackboard. Its low output leaves the closed basement almost black.
    URectLightComponent* ScreenStandby = CreateDefaultSubobject<URectLightComponent>(TEXT("ScreenStandbyResidualLight"));
    ScreenStandby->SetupAttachment(SceneRoot);
    ScreenStandby->SetRelativeLocation(HumanityTeaching::Screen + FVector(0,-2,-HumanityTeaching::ScreenSize.Z/2+3));
    ScreenStandby->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    ScreenStandby->SetMobility(EComponentMobility::Movable);
    ScreenStandby->SetIntensityUnits(ELightUnits::Lumens);
    ScreenStandby->SetIntensity(ScreenStandbyIntensityLumens);
    ScreenStandby->SetSourceWidth(8.0f);
    ScreenStandby->SetSourceHeight(3.0f);
    ScreenStandby->SetAttenuationRadius(340.0f);
    ScreenStandby->SetLightColor(FLinearColor(0.56f, 0.72f, 1.0f));
    ScreenStandby->SetCastShadows(true);
    ResidualLights.Add(ScreenStandby);
}

void AHumanityTrinityRebuildLightingController::BeginPlay()
{
    Super::BeginPlay();

    UMaterialInterface* PanelMaterial = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/HumanityTrinityRebuild/Materials/M_PanelLight.M_PanelLight"));
    if (!PanelMaterial)
    {
        UE_LOG(LogTemp, Error, TEXT("HumanityTrinityRebuild: missing M_PanelLight; rerun the Unreal asset setup."));
    }

    for (UStaticMeshComponent* Housing : PanelHousings)
    {
        if (Housing && PanelMaterial)
        {
            Housing->SetMaterial(0, PanelMaterial);
            if (UMaterialInstanceDynamic* HousingMaterial = Housing->CreateAndSetMaterialInstanceDynamic(0))
            {
                HousingMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.74f, 0.76f, 0.75f));
                HousingMaterial->SetScalarParameterValue(TEXT("Emission"), 0.0f);
            }
        }
    }

    PanelMaterials.Reset();
    for (UStaticMeshComponent* Panel : PanelVisuals)
    {
        if (Panel)
        {
            if (PanelMaterial)
            {
                Panel->SetMaterial(0, PanelMaterial);
            }
            if (UMaterialInstanceDynamic* DynamicMaterial = Panel->CreateAndSetMaterialInstanceDynamic(0))
            {
                PanelMaterials.Add(DynamicMaterial);
            }
            else
            {
                PanelMaterials.Add(nullptr);
            }
        }
    }

    ApplyLightingState();
}

void AHumanityTrinityRebuildLightingController::ToggleMaster()
{
    SetMasterLights(!AreMainLightsOn());
}

void AHumanityTrinityRebuildLightingController::SetMasterLights(const bool bEnabled)
{
    bMasterLightsOn = bEnabled;
    if (bEnabled && !bFrontZoneOn && !bMiddleZoneOn && !bRearZoneOn && !bStageZoneOn)
    {
        bFrontZoneOn = bMiddleZoneOn = bRearZoneOn = bStageZoneOn = true;
    }
    ApplyLightingState();

    if (GEngine && bShowStateFeedback)
    {
        const FString Message = bMasterLightsOn
            ? TEXT("MAIN LIGHTS: ON  |  Eye adaptation returning to normal")
            : TEXT("MAIN LIGHTS: OFF |  Dark adaptation begins");
        GEngine->AddOnScreenDebugMessage(2100, 3.0f, bMasterLightsOn ? FColor::Cyan : FColor::Orange, Message);
    }
}

void AHumanityTrinityRebuildLightingController::ToggleZone(const EHumanityTrinityRebuildLightZone Zone)
{
    SetZoneEnabled(Zone, !(bMasterLightsOn && IsZoneEnabled(Zone)));
}

void AHumanityTrinityRebuildLightingController::SetZoneEnabled(const EHumanityTrinityRebuildLightZone Zone, const bool bEnabled)
{
    if (bEnabled && !bMasterLightsOn)
    {
        // Selecting a circuit from blackout should illuminate only that
        // circuit, instead of unexpectedly restoring every remembered zone.
        bFrontZoneOn = bMiddleZoneOn = bRearZoneOn = bStageZoneOn = false;
        bMasterLightsOn = true;
    }
    GetMutableZoneState(Zone) = bEnabled;
    ApplyLightingState();
}

bool AHumanityTrinityRebuildLightingController::IsZoneEnabled(const EHumanityTrinityRebuildLightZone Zone) const
{
    switch (Zone)
    {
        case EHumanityTrinityRebuildLightZone::Front:  return bFrontZoneOn;
        case EHumanityTrinityRebuildLightZone::Middle: return bMiddleZoneOn;
        case EHumanityTrinityRebuildLightZone::Rear:   return bRearZoneOn;
        case EHumanityTrinityRebuildLightZone::Stage:  return bStageZoneOn;
        default:                      return true;
    }
}

int32 AHumanityTrinityRebuildLightingController::GetActiveMainLightCount() const
{
    int32 Count = 0;
    for (const URectLightComponent* Light : MainLights)
    {
        if (Light && Light->IsVisible() && Light->Intensity > KINDA_SMALL_NUMBER)
        {
            ++Count;
        }
    }
    return Count;
}

int32 AHumanityTrinityRebuildLightingController::GetEmittingPanelCount() const
{
    int32 Count = 0;
    for (const UMaterialInstanceDynamic* Material : PanelMaterials)
    {
        float Emission = 0.0f;
        if (Material && Material->GetScalarParameterValue(FMaterialParameterInfo(TEXT("Emission")), Emission) && Emission > 0.0f)
        {
            ++Count;
        }
    }
    return Count;
}

int32 AHumanityTrinityRebuildLightingController::GetActiveResidualLightCount() const
{
    int32 Count = 0;
    for (const URectLightComponent* Light : ResidualLights)
    {
        if (Light && Light->IsVisible() && Light->Intensity > KINDA_SMALL_NUMBER)
        {
            ++Count;
        }
    }
    return Count;
}

int32 AHumanityTrinityRebuildLightingController::GetActiveCeilingBounceCount() const
{
    int32 Count = 0;
    for (const URectLightComponent* Light : CeilingBounceLights)
        if (Light && Light->IsVisible() && Light->Intensity > 0.0f) ++Count;
    return Count;
}

bool& AHumanityTrinityRebuildLightingController::GetMutableZoneState(const EHumanityTrinityRebuildLightZone Zone)
{
    switch (Zone)
    {
        case EHumanityTrinityRebuildLightZone::Front:  return bFrontZoneOn;
        case EHumanityTrinityRebuildLightZone::Middle: return bMiddleZoneOn;
        case EHumanityTrinityRebuildLightZone::Rear:   return bRearZoneOn;
        case EHumanityTrinityRebuildLightZone::Stage:  return bStageZoneOn;
        default:                      return bFrontZoneOn;
    }
}

void AHumanityTrinityRebuildLightingController::ApplyLightingState()
{
    for (int32 Index = 0; Index < MainLights.Num(); ++Index)
    {
        if (URectLightComponent* Light = MainLights[Index])
        {
            const bool bEnabled = bMasterLightsOn && IsZoneEnabled(MainLightZones[Index]);
            const bool bIsStage = MainLightZones[Index] == EHumanityTrinityRebuildLightZone::Stage;
            const float CircuitIntensity = bIsStage ? StageLightIntensityLumens : MainLightIntensityLumens;
            Light->SetIntensity(bEnabled ? CircuitIntensity : 0.0f);
            Light->SetTemperature(bIsStage ? StageTemperatureKelvin : ClassroomTemperatureKelvin);
            Light->SetVisibility(bEnabled, true);
            URectLightComponent* Bounce = CeilingBounceLights[Index];
            Bounce->SetIntensity(bEnabled ? CircuitIntensity*CeilingBounceFraction : 0.0f);
            Bounce->SetTemperature(bIsStage ? StageTemperatureKelvin : ClassroomTemperatureKelvin);
            Bounce->SetVisibility(bEnabled, true);
        }
    }

    for (int32 Index = 0; Index < PanelVisuals.Num(); ++Index)
    {
        const bool bEnabled = bMasterLightsOn && IsZoneEnabled(PanelZones[Index]);
        if (UStaticMeshComponent* Panel = PanelVisuals[Index])
        {
            Panel->SetVisibility(true, true);
        }
        if (PanelMaterials.IsValidIndex(Index) && PanelMaterials[Index])
        {
            PanelMaterials[Index]->SetVectorParameterValue(
                TEXT("Color"),
                PanelZones[Index] == EHumanityTrinityRebuildLightZone::Stage
                    ? FLinearColor(1.0f, 0.90f, 0.75f) : FLinearColor(0.94f, 0.97f, 1.0f));
            PanelMaterials[Index]->SetScalarParameterValue(TEXT("Emission"), bEnabled ? PanelEmission : 0.0f);
        }
    }

    for (int32 Index = 0; Index < ResidualLights.Num(); ++Index)
    {
        if (URectLightComponent* Light = ResidualLights[Index])
        {
            const bool bResidualVisible = !AreMainLightsOn() && bResidualLightsEnabled;
            const float Intensity = (Index == 0) ? DoorLeakIntensityLumens : ScreenStandbyIntensityLumens;
            Light->SetIntensity(bResidualVisible ? Intensity : 0.0f);
            Light->SetVisibility(bResidualVisible, true);
        }
    }
}
