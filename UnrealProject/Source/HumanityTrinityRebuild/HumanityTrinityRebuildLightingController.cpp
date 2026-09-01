#include "HumanityTrinityRebuildLightingController.h"

#include "Components/RectLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
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
        { TEXT("FrontLight_Left"),   FVector(-300.0,  -260.0, 322.0), EHumanityTrinityRebuildLightZone::Front },
        { TEXT("FrontLight_Right"),  FVector( 300.0,  -260.0, 322.0), EHumanityTrinityRebuildLightZone::Front },
        { TEXT("MiddleLight_LeftA"), FVector(-300.0,  -650.0, 322.0), EHumanityTrinityRebuildLightZone::Middle },
        { TEXT("MiddleLight_RightA"),FVector( 300.0,  -650.0, 322.0), EHumanityTrinityRebuildLightZone::Middle },
        { TEXT("MiddleLight_LeftB"), FVector(-300.0,  -990.0, 322.0), EHumanityTrinityRebuildLightZone::Middle },
        { TEXT("MiddleLight_RightB"),FVector( 300.0,  -990.0, 322.0), EHumanityTrinityRebuildLightZone::Middle },
        { TEXT("RearLight_Left"),    FVector(-300.0, -1280.0, 322.0), EHumanityTrinityRebuildLightZone::Rear },
        { TEXT("RearLight_Right"),   FVector( 300.0, -1280.0, 322.0), EHumanityTrinityRebuildLightZone::Rear },
        { TEXT("StageLight"),        FVector(   0.0, -1580.0, 322.0), EHumanityTrinityRebuildLightZone::Stage }
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
        Light->SetSourceWidth(220.0f);
        Light->SetSourceHeight(90.0f);
        Light->SetAttenuationRadius(900.0f);
        Light->SetLightColor(FLinearColor(0.91f, 0.95f, 1.0f));
        Light->SetCastShadows(true);
        MainLights.Add(Light);
        MainLightZones.Add(Spec.Zone);
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
            UStaticMeshComponent* Panel = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
            Panel->SetupAttachment(SceneRoot);
            Panel->SetStaticMesh(CubeMesh);
            Panel->SetRelativeLocation(FVector(PanelX[Column], PanelY[Row], 329.0));
            Panel->SetRelativeScale3D(FVector(1.25, 0.32, 0.045));
            Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Panel->SetCastShadow(false);
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

    // The screen is on the front teaching wall and therefore throws its very
    // weak standby light down the length of the otherwise sealed room.
    URectLightComponent* ScreenStandby = CreateDefaultSubobject<URectLightComponent>(TEXT("ScreenStandbyResidualLight"));
    ScreenStandby->SetupAttachment(SceneRoot);
    ScreenStandby->SetRelativeLocation(FVector(0.0, -45.0, 190.0));
    ScreenStandby->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    ScreenStandby->SetMobility(EComponentMobility::Movable);
    ScreenStandby->SetIntensityUnits(ELightUnits::Lumens);
    ScreenStandby->SetIntensity(ScreenStandbyIntensityLumens);
    ScreenStandby->SetSourceWidth(220.0f);
    ScreenStandby->SetSourceHeight(120.0f);
    ScreenStandby->SetAttenuationRadius(900.0f);
    ScreenStandby->SetLightColor(FLinearColor(0.12f, 0.35f, 1.0f));
    ScreenStandby->SetCastShadows(false);
    ResidualLights.Add(ScreenStandby);
}

void AHumanityTrinityRebuildLightingController::BeginPlay()
{
    Super::BeginPlay();

    for (UStaticMeshComponent* Panel : PanelVisuals)
    {
        if (Panel)
        {
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
    SetMasterLights(!bMasterLightsOn);
}

void AHumanityTrinityRebuildLightingController::SetMasterLights(const bool bEnabled)
{
    bMasterLightsOn = bEnabled;
    ApplyLightingState();

    if (GEngine)
    {
        const FString Message = bMasterLightsOn
            ? TEXT("MAIN LIGHTS: ON  |  Eye adaptation returning to normal")
            : TEXT("MAIN LIGHTS: OFF |  Dark adaptation begins");
        GEngine->AddOnScreenDebugMessage(2100, 3.0f, bMasterLightsOn ? FColor::Cyan : FColor::Orange, Message);
    }
}

void AHumanityTrinityRebuildLightingController::ToggleZone(const EHumanityTrinityRebuildLightZone Zone)
{
    bool& ZoneState = GetMutableZoneState(Zone);
    ZoneState = !ZoneState;
    ApplyLightingState();
}

void AHumanityTrinityRebuildLightingController::SetZoneEnabled(const EHumanityTrinityRebuildLightZone Zone, const bool bEnabled)
{
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
        if (Light && Light->IsVisible())
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
        if (Light && Light->IsVisible())
        {
            ++Count;
        }
    }
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
            Light->SetIntensity(bEnabled ? MainLightIntensityLumens : 0.0f);
            Light->SetVisibility(bEnabled, true);
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
                bEnabled ? FLinearColor(0.95f, 0.98f, 1.0f) : FLinearColor(0.025f, 0.03f, 0.04f));
        }
    }

    for (int32 Index = 0; Index < ResidualLights.Num(); ++Index)
    {
        if (URectLightComponent* Light = ResidualLights[Index])
        {
            const bool bResidualVisible = !bMasterLightsOn && bResidualLightsEnabled;
            const float Intensity = (Index == 0) ? DoorLeakIntensityLumens : ScreenStandbyIntensityLumens;
            Light->SetIntensity(bResidualVisible ? Intensity : 0.0f);
            Light->SetVisibility(bResidualVisible, true);
        }
    }
}
