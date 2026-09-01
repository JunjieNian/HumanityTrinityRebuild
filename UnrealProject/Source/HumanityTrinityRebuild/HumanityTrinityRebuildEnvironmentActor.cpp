#include "HumanityTrinityRebuildEnvironmentActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AHumanityTrinityRebuildEnvironmentActor::AHumanityTrinityRebuildEnvironmentActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("EnvironmentRoot"));
    RootComponent = SceneRoot;

    ImportedEnvironment = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ImportedHumanityTrinityRebuildEnvironment"));
    ImportedEnvironment->SetupAttachment(SceneRoot);
    ImportedEnvironment->SetMobility(EComponentMobility::Static);
    ImportedEnvironment->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ImportedEnvironment->SetCollisionResponseToAllChannels(ECR_Block);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    FallbackCubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
}

void AHumanityTrinityRebuildEnvironmentActor::BeginPlay()
{
    Super::BeginPlay();

    UStaticMesh* EnvironmentMesh = LoadObject<UStaticMesh>(
        nullptr,
        TEXT("/Game/HumanityTrinityRebuild/Environment/HumanityTrinityRebuildEnvironment_Runtime.HumanityTrinityRebuildEnvironment_Runtime"));

    if (EnvironmentMesh)
    {
        ImportedEnvironment->SetStaticMesh(EnvironmentMesh);
        ImportedEnvironment->SetVisibility(true);
        UE_LOG(LogTemp, Display, TEXT("Loaded HumanityTrinityRebuild environment mesh."));
    }
    else
    {
        ImportedEnvironment->SetVisibility(false);
        BuildFallbackRoom();

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                2200,
                8.0f,
                FColor::Yellow,
                TEXT("Imported model not found; showing fallback room. Run setup_humanity_trinity_rebuild_unreal.py in the Unreal Editor."));
        }
    }
}

void AHumanityTrinityRebuildEnvironmentActor::BuildFallbackRoom()
{
    if (!FallbackCubeMesh)
    {
        return;
    }

    // Cube mesh is 100 cm on each side. These scales produce a 12 m x 18 m x 3.4 m sealed room.
    AddFallbackBox(TEXT("FallbackFloor"),   FVector(0.0, -900.0,  -5.0), FVector(12.0, 18.0, 0.10));
    AddFallbackBox(TEXT("FallbackCeiling"), FVector(0.0, -900.0, 345.0), FVector(12.0, 18.0, 0.10));
    AddFallbackBox(TEXT("FallbackFront"),   FVector(0.0,    5.0, 170.0), FVector(12.0, 0.10, 3.40));
    AddFallbackBox(TEXT("FallbackBack"),    FVector(0.0,-1805.0, 170.0), FVector(12.0, 0.10, 3.40));
    AddFallbackBox(TEXT("FallbackLeft"),    FVector(-605.0,-900.0,170.0), FVector(0.10,18.0,3.40));
    AddFallbackBox(TEXT("FallbackRight"),   FVector( 605.0,-900.0,170.0), FVector(0.10,18.0,3.40));
}

void AHumanityTrinityRebuildEnvironmentActor::AddFallbackBox(
    const TCHAR* Name,
    const FVector& Location,
    const FVector& Scale)
{
    UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, Name);
    Part->SetStaticMesh(FallbackCubeMesh);
    Part->SetupAttachment(SceneRoot);
    Part->SetRelativeLocation(Location);
    Part->SetRelativeScale3D(Scale);
    Part->SetMobility(EComponentMobility::Static);
    Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Part->SetCollisionResponseToAllChannels(ECR_Block);
    Part->RegisterComponent();
    FallbackParts.Add(Part);
}
