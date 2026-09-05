#include "HumanityTrinityRebuildRoomInteraction.h"
#include "HumanityTrinityRebuildDoorLayout.h"
#include "Engine/World.h"

#include "Components/BoxComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AHumanityTrinityRebuildRoomInteraction::AHumanityTrinityRebuildRoomInteraction()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RoomInteractionRoot"));
    RootComponent = SceneRoot;

    LeftCurtain = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftCurtain"));
    RightCurtain = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightCurtain"));
    LeftCurtainBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftCurtainBounds"));
    RightCurtainBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("RightCurtainBounds"));
    for (UStaticMeshComponent* Panel : {LeftCurtain.Get(), RightCurtain.Get()})
    {
        Panel->SetupAttachment(SceneRoot);
        Panel->SetMobility(EComponentMobility::Movable);
        Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Panel->bCastShadowAsTwoSided = true;
    }
    for (UBoxComponent* Bounds : {LeftCurtainBounds.Get(), RightCurtainBounds.Get()})
    {
        Bounds->SetupAttachment(SceneRoot);
        Bounds->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Bounds->SetCollisionResponseToAllChannels(ECR_Block);
        Bounds->CanCharacterStepUpOn = ECB_No;
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterialFinder(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    UStaticMesh* Cube = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
    UMaterialInterface* BasicMaterial = BasicMaterialFinder.Succeeded() ? BasicMaterialFinder.Object : nullptr;

    for (int32 Index = 0; Index < 2; ++Index)
    {
        const FHumanityPropDoorSpec& Spec = HumanityPropDoors[Index];
        USceneComponent* Pivot = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("PropDoorPivot%d"), Index));
        Pivot->SetupAttachment(SceneRoot);
        Pivot->SetRelativeLocation(Spec.Hinge);
        Pivot->SetRelativeRotation(FRotator(0, Spec.Yaw, 0));
        UStaticMeshComponent* Leaf = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("PropDoorLeaf%d"), Index));
        Leaf->SetupAttachment(Pivot);
        Leaf->SetStaticMesh(Cube);
        Leaf->SetMobility(EComponentMobility::Movable);
        Leaf->SetRelativeLocation(FVector(Spec.Width/2, 0, Spec.Height/2));
        Leaf->SetRelativeScale3D(FVector(Spec.Width, Spec.Thickness, Spec.Height)/100.0f);
        Leaf->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Leaf->SetCollisionResponseToAllChannels(ECR_Block);
        Leaf->CanCharacterStepUpOn = ECB_No;
        PropDoorPivots.Add(Pivot);
        PropDoorLeaves.Add(Leaf);
        PropDoorFractions.Add(0.0f);
        PropDoorTargets.Add(false);

    }

    ScreenFace = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeachingDisplay"));
    ScreenFace->SetupAttachment(SceneRoot);
    ScreenFace->SetStaticMesh(Cube);
    ScreenFace->SetMaterial(0, BasicMaterial);
    ScreenFace->SetRelativeLocation(FVector(0.0f, -24.0f, 185.0f));
    ScreenFace->SetRelativeScale3D(FVector(2.50f, 0.01f, 1.38f));
    ScreenFace->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ScreenFace->SetCollisionResponseToAllChannels(ECR_Ignore);
    ScreenFace->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    ScreenFace->SetCastShadow(false);
    ScreenFace->SetVisibility(false);

    // A small controller on the lectern is reachable without having to touch
    // the teaching display. This position follows the mirrored Blender podium.
    ScreenControlBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("DisplayControlBounds"));
    ScreenControlBounds->SetupAttachment(SceneRoot);
    ScreenControlBounds->SetRelativeLocation(FVector(-358.0f, -102.0f, 113.0f));
    ScreenControlBounds->SetBoxExtent(FVector(12.0f, 11.0f, 5.0f));
    ScreenControlBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ScreenControlBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    ScreenControlBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    ScreenControl = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DisplayControl"));
    ScreenControl->SetupAttachment(ScreenControlBounds);
    ScreenControl->SetStaticMesh(Cube);
    ScreenControl->SetMaterial(0, BasicMaterial);
    ScreenControl->SetRelativeScale3D(FVector(0.14f, 0.10f, 0.02f));
    ScreenControl->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ScreenHeading = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DisplayHeading"));
    ScreenHeading->SetupAttachment(SceneRoot);
    ScreenHeading->SetRelativeLocation(FVector(0.0f, -25.0f, 204.0f));
    ScreenHeading->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    ScreenHeading->SetHorizontalAlignment(EHTA_Center);
    ScreenHeading->SetWorldSize(9.5f);
    ScreenHeading->SetText(FText::FromString(TEXT("HUMANITY TRINITY")));
    ScreenHeading->SetVisibility(false);
    ScreenHeading->SetCastShadow(false);

    ScreenCaption = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DisplayCaption"));
    ScreenCaption->SetupAttachment(SceneRoot);
    ScreenCaption->SetRelativeLocation(FVector(0.0f, -25.0f, 176.0f));
    ScreenCaption->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    ScreenCaption->SetHorizontalAlignment(EHTA_Center);
    ScreenCaption->SetWorldSize(5.0f);
    ScreenCaption->SetText(FText::FromString(TEXT("READ  /  DISCUSS  /  PERFORM")));
    ScreenCaption->SetVisibility(false);
    ScreenCaption->SetCastShadow(false);

    ScreenLight = CreateDefaultSubobject<URectLightComponent>(TEXT("DisplayLocalLight"));
    ScreenLight->SetupAttachment(SceneRoot);
    ScreenLight->SetRelativeLocation(FVector(0.0f, -30.0f, 185.0f));
    ScreenLight->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    ScreenLight->SetMobility(EComponentMobility::Movable);
    ScreenLight->SetIntensityUnits(ELightUnits::Lumens);
    ScreenLight->SetIntensity(300.0f);
    ScreenLight->SetLightColor(FLinearColor(0.68f, 0.82f, 1.0f));
    ScreenLight->SetSourceWidth(220.0f);
    ScreenLight->SetSourceHeight(120.0f);
    ScreenLight->SetAttenuationRadius(500.0f);
    ScreenLight->SetCastShadows(true);
    ScreenLight->SetVisibility(false);
}

void AHumanityTrinityRebuildRoomInteraction::BeginPlay()
{
    Super::BeginPlay();
    UStaticMesh* CurtainMesh = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/HumanityTrinityRebuild/Interactive/SM_CurtainPanel.SM_CurtainPanel"));
    bHasCurtainMesh = CurtainMesh != nullptr;
    if (CurtainMesh)
    {
        LeftCurtain->SetStaticMesh(CurtainMesh);
        RightCurtain->SetStaticMesh(CurtainMesh);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Curtain mesh missing; run the project asset setup script."));
        LeftCurtainBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        RightCurtainBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    UMaterialInterface* DisplayMaterial = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/HumanityTrinityRebuild/Materials/M_TeachingDisplay.M_TeachingDisplay"));
    if (DisplayMaterial)
    {
        ScreenFace->SetMaterial(0, DisplayMaterial);
    }
    ScreenMaterial = ScreenFace->CreateAndSetMaterialInstanceDynamic(0);
    ControlMaterial = ScreenControl->CreateAndSetMaterialInstanceDynamic(0);
    UMaterialInterface* Wood = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/HumanityTrinityRebuild/Materials/Surfaces/M_Surface_Acoustic_WarmOak.M_Surface_Acoustic_WarmOak"));
    for (UStaticMeshComponent* Leaf : PropDoorLeaves)
    {
        Leaf->SetMaterial(0, Wood);
    }
    UpdateCurtainGeometry();
    SetScreenOn(false);
}

void AHumanityTrinityRebuildRoomInteraction::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    for (int32 Index = 0; Index < PropDoorLeaves.Num(); ++Index)
    {
        const FHumanityPropDoorSpec& Spec = HumanityPropDoors[Index];
        const float Next = FMath::FInterpConstantTo(PropDoorFractions[Index], PropDoorTargets[Index] ? 1.0f : 0.0f,
            DeltaSeconds, 1.0f);
        const FQuat Rotation = FRotator(0, Spec.Yaw + Spec.Swing*FMath::SmoothStep(0.0f,1.0f,Next), 0).Quaternion();
        const FVector Centre = Spec.Hinge + Rotation.RotateVector(FVector(Spec.Width/2,0,Spec.Height/2));
        // Pause the leaf when the visitor occupies its next position; closing a
        // concealed door must never sweep the player through the partition.
        FCollisionObjectQueryParams Objects;
        Objects.AddObjectTypesToQuery(ECC_Pawn);
        FCollisionQueryParams Query(SCENE_QUERY_STAT(PropDoorVisitor), false, this);
        if (!GetWorld()->OverlapAnyTestByObjectType(Centre, Rotation, Objects,
            FCollisionShape::MakeBox(FVector(Spec.Width/2,Spec.Thickness/2+2,Spec.Height/2)), Query))
        {
            PropDoorFractions[Index] = Next;
            PropDoorPivots[Index]->SetRelativeRotation(Rotation);
        }
    }
    const float Target = bCurtainsTargetOpen ? 1.0f : 0.0f;
    if (!FMath::IsNearlyEqual(CurtainOpenFraction, Target))
    {
        CurtainOpenFraction = FMath::FInterpConstantTo(CurtainOpenFraction, Target, DeltaSeconds,
            1.0f / FMath::Max(CurtainTravelSeconds, 0.2f));
        UpdateCurtainGeometry();
    }
}

void AHumanityTrinityRebuildRoomInteraction::UpdateCurtainGeometry()
{
    // The imported pleated mesh is one metre wide/high, anchored at x=0.
    // Smoothstep gives the drape a gentle start/stop while its outer edge stays
    // fixed beside the stage. Collision follows the actual leading edge.
    const float EasedOpen = FMath::SmoothStep(0.0f, 1.0f, CurtainOpenFraction);
    const float Width = FMath::Lerp(570.0f, 110.0f, EasedOpen);
    LeftCurtain->SetRelativeLocation(FVector(-570.0f, -1436.0f, 21.0f));
    RightCurtain->SetRelativeLocation(FVector(570.0f, -1436.0f, 21.0f));
    LeftCurtain->SetRelativeScale3D(FVector(Width / 100.0f, 1.0f, 3.0f));
    RightCurtain->SetRelativeScale3D(FVector(-Width / 100.0f, 1.0f, 3.0f));
    LeftCurtainBounds->SetRelativeLocation(FVector(-570.0f + Width * 0.5f, -1436.0f, 171.0f));
    RightCurtainBounds->SetRelativeLocation(FVector(570.0f - Width * 0.5f, -1436.0f, 171.0f));
    LeftCurtainBounds->SetBoxExtent(FVector(Width * 0.5f, 7.0f, 150.0f));
    RightCurtainBounds->SetBoxExtent(FVector(Width * 0.5f, 7.0f, 150.0f));
}

void AHumanityTrinityRebuildRoomInteraction::ToggleCurtains()
{
    SetCurtainsOpen(!bCurtainsTargetOpen);
}

void AHumanityTrinityRebuildRoomInteraction::SetCurtainsOpen(const bool bOpen)
{
    bCurtainsTargetOpen = bOpen;
}

void AHumanityTrinityRebuildRoomInteraction::ToggleScreen()
{
    SetScreenOn(!bScreenOn);
}

void AHumanityTrinityRebuildRoomInteraction::SetScreenOn(const bool bOn)
{
    bScreenOn = bOn;
    ScreenFace->SetVisibility(bOn);
    ScreenFace->SetCollisionEnabled(bOn ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    ScreenHeading->SetVisibility(bOn);
    ScreenCaption->SetVisibility(bOn);
    ScreenLight->SetVisibility(bOn);
    if (ScreenMaterial)
    {
        ScreenMaterial->SetScalarParameterValue(TEXT("Emission"), bOn ? 1.0f : 0.0f);
        ScreenMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.055f, 0.14f, 0.24f));
    }
    if (ControlMaterial)
    {
        ControlMaterial->SetVectorParameterValue(TEXT("Color"), bOn
            ? FLinearColor(0.10f, 0.20f, 0.24f) : FLinearColor(0.04f, 0.045f, 0.05f));
    }
}

bool AHumanityTrinityRebuildRoomInteraction::IsScreenIlluminating() const
{
    return bScreenOn && ScreenLight->IsVisible() && ScreenLight->Intensity > 0.0f;
}

bool AHumanityTrinityRebuildRoomInteraction::IsCurtainComponent(const UPrimitiveComponent* Component) const
{
    return Component == LeftCurtainBounds || Component == RightCurtainBounds;
}

bool AHumanityTrinityRebuildRoomInteraction::IsScreenComponent(const UPrimitiveComponent* Component) const
{
    return Component == ScreenControlBounds || Component == ScreenFace;
}

FString AHumanityTrinityRebuildRoomInteraction::GetInteractionPrompt(const UPrimitiveComponent* Component) const
{
    const int32 DoorIndex = GetPropDoorIndex(Component);
    if (DoorIndex != INDEX_NONE)
    {
        return PropDoorTargets[DoorIndex] ? TEXT("E - Close concealed prop-room door") : TEXT("E - Open concealed prop-room door");
    }
    if (IsCurtainComponent(Component))
    {
        return bCurtainsTargetOpen ? TEXT("E - Close stage curtains") : TEXT("E - Open stage curtains");
    }
    if (IsScreenComponent(Component))
    {
        return bScreenOn ? TEXT("E - Turn OFF teaching display") : TEXT("E - Turn ON teaching display");
    }
    return FString();
}

void AHumanityTrinityRebuildRoomInteraction::Interact(UPrimitiveComponent* Component)
{
    const int32 DoorIndex = GetPropDoorIndex(Component);
    if (DoorIndex != INDEX_NONE)
    {
        SetPropDoorOpen(DoorIndex, !PropDoorTargets[DoorIndex]);
    }
    if (IsCurtainComponent(Component))
    {
        ToggleCurtains();
    }
    else if (IsScreenComponent(Component))
    {
        ToggleScreen();
    }
}

int32 AHumanityTrinityRebuildRoomInteraction::GetPropDoorIndex(const UPrimitiveComponent* Component) const
{
    for (int32 Index = 0; Index < PropDoorLeaves.Num(); ++Index)
    {
        if (Component == PropDoorLeaves[Index]) return Index;
    }
    return INDEX_NONE;
}

void AHumanityTrinityRebuildRoomInteraction::SetPropDoorOpen(int32 Index, bool bOpen)
{
    if (PropDoorTargets.IsValidIndex(Index)) PropDoorTargets[Index] = bOpen;
}

float AHumanityTrinityRebuildRoomInteraction::GetPropDoorOpenFraction(int32 Index) const
{
    return PropDoorFractions.IsValidIndex(Index) ? PropDoorFractions[Index] : -1.0f;
}
