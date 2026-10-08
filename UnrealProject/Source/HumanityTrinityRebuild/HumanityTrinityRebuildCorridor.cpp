#include "HumanityTrinityRebuildCorridor.h"
#include "HumanityTrinityRebuildCorridorLayout.h"
#include "Components/RectLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AHumanityTrinityRebuildCorridor::AHumanityTrinityRebuildCorridor()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CorridorRoot"));
    RootComponent = SceneRoot;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    UStaticMesh* Cube = CubeFinder.Object;
    for (const auto& Spec : HumanityCorridor::Parts)
    {
        auto* Part = CreateDefaultSubobject<UStaticMeshComponent>(Spec.Name);
        Part->SetupAttachment(SceneRoot);
        Part->SetStaticMesh(Cube);
        Part->SetMaterial(0, MaterialFinder.Object);
        Part->SetRelativeLocation(Spec.Position);
        Part->SetRelativeScale3D(Spec.Size / 100.f);
        Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Part->SetCollisionResponseToAllChannels(ECR_Block);
        Part->SetCastShadow(Spec.Material != 6);
        Part->CanCharacterStepUpOn = Spec.Material == 1 ? ECB_Yes : ECB_No;
        CorridorParts.Add(Part);
    }
    // These lights remain physically in the corridor in every room-light state.
    // Opaque classroom walls and animated door leaves cast the actual doorway shadows.
    for (int32 I = 0; I < 4; ++I)
    {
        auto* Light = CreateDefaultSubobject<URectLightComponent>(*FString::Printf(TEXT("CorridorLight%d"), I));
        Light->SetupAttachment(SceneRoot);
        Light->SetRelativeLocation(FVector(-759, -225 - 425 * I, 311));
        Light->SetRelativeRotation(FRotator(-90, 0, 0));
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetIntensityUnits(ELightUnits::Lumens);
        Light->SetIntensity(1900);
        Light->SetLightColor(FLinearColor(.76f,.86f,1.f));
        Light->SetAttenuationRadius(1100);
        Light->SetSourceWidth(112);
        Light->SetSourceHeight(58);
        Light->SetCastShadows(true);
        CorridorLights.Add(Light);
    }
    for (int32 I = 0; I < UE_ARRAY_COUNT(HumanityCorridor::Doors); ++I)
    {
        const auto& Spec = HumanityCorridor::Doors[I];
        auto* Pivot = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("ExteriorDoorPivot%d"), I));
        Pivot->SetupAttachment(SceneRoot);
        Pivot->SetRelativeLocation(Spec.Hinge);
        Pivot->SetRelativeRotation(FRotator(0,Spec.Yaw,0));
        auto* Leaf = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ExteriorDoorLeaf%d"), I));
        Leaf->SetupAttachment(Pivot);
        Leaf->SetStaticMesh(Cube);
        Leaf->SetMaterial(0,MaterialFinder.Object);
        Leaf->SetRelativeLocation(FVector(Spec.Width/2,0,Spec.Height/2));
        Leaf->SetRelativeScale3D(FVector(Spec.Width,Spec.Thickness,Spec.Height)/100.f);
        Leaf->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Leaf->SetCollisionResponseToAllChannels(ECR_Block);
        Leaf->CanCharacterStepUpOn = ECB_No;
        Leaf->SetMobility(EComponentMobility::Movable);
        Leaf->SetCastShadow(true);
        DoorLeaves.Add(Leaf); DoorPivots.Add(Pivot);
        DoorFractions.Add(0.f); DoorTargets.Add(false);
        for (int32 Side : {-1,1})
        {
            auto* Plate = CreateDefaultSubobject<UStaticMeshComponent>(
                *FString::Printf(TEXT("DoorHandlePlate%d_%d"),I,Side));
            Plate->SetupAttachment(Pivot);
            Plate->SetStaticMesh(Cube); Plate->SetMaterial(0,MaterialFinder.Object);
            Plate->SetRelativeLocation(FVector(Spec.Width-12,Side*3.2f,105));
            Plate->SetRelativeScale3D(FVector(.055f,.013f,.17f));
            Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            DoorHardware.Add(Plate);
            auto* Handle = CreateDefaultSubobject<UStaticMeshComponent>(
                *FString::Printf(TEXT("DoorHandle%d_%d"),I,Side));
            Handle->SetupAttachment(Pivot);
            Handle->SetStaticMesh(Cube); Handle->SetMaterial(0,MaterialFinder.Object);
            Handle->SetRelativeLocation(FVector(Spec.Width-17,Side*5.f,106));
            Handle->SetRelativeScale3D(FVector(.15f,.024f,.024f));
            Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            DoorHardware.Add(Handle);
        }
        auto* Plaque = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("DoorLabel%d"),I));
        Plaque->SetupAttachment(SceneRoot);
        Plaque->SetRelativeLocation(FVector(-630,Spec.Center.Y,244));
        Plaque->SetRelativeRotation(FRotator(0,180,0));
        Plaque->SetHorizontalAlignment(EHTA_Center);
        Plaque->SetWorldSize(8.f);
        Plaque->SetText(FText::FromString(I == 2 ? TEXT("PROP ROOM") : TEXT("HUMANITY TRINITY")));
    }
}

void AHumanityTrinityRebuildCorridor::BeginPlay()
{
    Super::BeginPlay();
    UMaterialInterface* Panel = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/HumanityTrinityRebuild/Materials/M_PanelLight.M_PanelLight"));
    UMaterialInterface* Wood = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/HumanityTrinityRebuild/Materials/Surfaces/M_Surface_Door_Wood.M_Surface_Door_Wood"));
    if (!Wood) Wood = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/HumanityTrinityRebuild/Materials/Surfaces/M_Surface_Acoustic_WarmOak.M_Surface_Acoustic_WarmOak"));
    for (int32 I = 0; I < CorridorParts.Num(); ++I)
    {
        const int32 MaterialIndex = HumanityCorridor::Parts[I].Material;
        if (Panel) CorridorParts[I]->SetMaterial(0,Panel);
        if (auto* Material = CorridorParts[I]->CreateAndSetMaterialInstanceDynamic(0))
        {
            Material->SetVectorParameterValue(TEXT("Color"),HumanityCorridor::Colors[MaterialIndex]);
            Material->SetScalarParameterValue(TEXT("Emission"),MaterialIndex == 6 ? 4.f : 0.f);
        }
    }
    for (UStaticMeshComponent* Leaf : DoorLeaves) if (Wood) Leaf->SetMaterial(0,Wood);
    for (UStaticMeshComponent* Part : DoorHardware)
        if (auto* Material=Part->CreateAndSetMaterialInstanceDynamic(0))
            Material->SetVectorParameterValue(TEXT("Color"),FLinearColor(.36f,.39f,.40f));
    UE_LOG(LogTemp,Display,TEXT("[TEACHER_PATROL] CORRIDOR_READY doors=3 default_closed=YES lights=%d parts=%d"),
        CorridorLights.Num(),CorridorParts.Num());
}

void AHumanityTrinityRebuildCorridor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    for (int32 I=0; I<DoorLeaves.Num(); ++I)
    {
        const float Next=FMath::FInterpConstantTo(DoorFractions[I],DoorTargets[I] ? 1.f : 0.f,DeltaSeconds,.78f);
        if (FMath::IsNearlyEqual(Next,DoorFractions[I])) continue;
        const auto& Spec=HumanityCorridor::Doors[I];
        const FQuat Rotation=FRotator(0,Spec.Yaw+Spec.Swing*FMath::SmoothStep(0.f,1.f,Next),0).Quaternion();
        const FVector Centre=GetActorTransform().TransformPosition(
            Spec.Hinge+Rotation.RotateVector(FVector(Spec.Width/2,0,Spec.Height/2)));
        FCollisionObjectQueryParams Objects;
        Objects.AddObjectTypesToQuery(ECC_Pawn);
        FCollisionQueryParams Query(SCENE_QUERY_STAT(ExteriorDoorVisitor),false,this);
        // A stopped/reversed leaf cannot push a visitor out through the wall.
        if (!GetWorld()->OverlapAnyTestByObjectType(Centre,GetActorQuat()*Rotation,Objects,
            FCollisionShape::MakeBox(FVector(Spec.Width/2,Spec.Thickness/2+2,Spec.Height/2)),Query))
        {
            DoorFractions[I]=Next;
            DoorPivots[I]->SetRelativeRotation(Rotation);
        }
    }
}

AHumanityTrinityRebuildCorridor* AHumanityTrinityRebuildCorridor::Find(UWorld* World)
{
    if (World) for (TActorIterator<AHumanityTrinityRebuildCorridor> It(World); It; ++It) return *It;
    return nullptr;
}
void AHumanityTrinityRebuildCorridor::SetDoorOpen(int32 Index,bool bOpen)
{
    if (DoorTargets.IsValidIndex(Index)) DoorTargets[Index]=bOpen;
}
float AHumanityTrinityRebuildCorridor::GetDoorOpenFraction(int32 Index) const
{
    return DoorFractions.IsValidIndex(Index) ? DoorFractions[Index] : 0.f;
}
int32 AHumanityTrinityRebuildCorridor::DoorIndexForComponent(const UPrimitiveComponent* Component) const
{
    for (int32 I=0; I<DoorLeaves.Num(); ++I) if (DoorLeaves[I]==Component) return I;
    return INDEX_NONE;
}
bool AHumanityTrinityRebuildCorridor::ToggleDoor(int32 Index)
{
    if (bPatrolDoorLocked || !DoorTargets.IsValidIndex(Index)) return false;
    DoorTargets[Index]=!DoorTargets[Index]; return true;
}
FString AHumanityTrinityRebuildCorridor::GetDoorPrompt(int32 Index) const
{
    if (!DoorTargets.IsValidIndex(Index)) return FString();
    if (bPatrolDoorLocked) return TEXT("老师正在巡查，不能开门 / Teacher patrol: door locked");
    return DoorTargets[Index] ? TEXT("E 关闭教室门 / Close classroom door") : TEXT("E 打开教室门 / Open classroom door");
}
FVector AHumanityTrinityRebuildCorridor::GetDoorwayLocation(int32 Index) const
{
    return GetActorTransform().TransformPosition(HumanityCorridor::Doors[FMath::Clamp(Index,0,2)].Center);
}
FVector AHumanityTrinityRebuildCorridor::GetCorridorApproachLocation() const
{
    return GetActorTransform().TransformPosition(HumanityCorridor::Approach);
}
FVector AHumanityTrinityRebuildCorridor::GetInspectionLocation() const
{
    return GetActorTransform().TransformPosition(HumanityCorridor::Inspection);
}
