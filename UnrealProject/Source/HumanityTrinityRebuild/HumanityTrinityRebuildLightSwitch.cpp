#include "HumanityTrinityRebuildLightSwitch.h"

#include "HumanityTrinityRebuildLightingController.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AHumanityTrinityRebuildLightSwitch::AHumanityTrinityRebuildLightSwitch()
{
    PrimaryActorTick.bCanEverTick = false;

    InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
    InteractionBounds->SetBoxExtent(FVector(12.0f, 24.0f, 34.0f));
    InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    RootComponent = InteractionBounds;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterialFinder(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    UMaterialInterface* BasicMaterial = BasicMaterialFinder.Succeeded() ? BasicMaterialFinder.Object : nullptr;

    BackPlate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackPlate"));
    BackPlate->SetupAttachment(InteractionBounds);
    BackPlate->SetStaticMesh(CubeMesh);
    BackPlate->SetMaterial(0, BasicMaterial);
    BackPlate->SetRelativeScale3D(FVector(0.08f, 0.22f, 0.32f));
    BackPlate->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Paddle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Paddle"));
    Paddle->SetupAttachment(InteractionBounds);
    Paddle->SetStaticMesh(CubeMesh);
    Paddle->SetMaterial(0, BasicMaterial);
    Paddle->SetRelativeLocation(FVector(-6.0f, 0.0f, 0.0f));
    Paddle->SetRelativeScale3D(FVector(0.055f, 0.14f, 0.12f));
    Paddle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(InteractionBounds);
    Label->SetText(FText::FromString(TEXT("LIGHTS")));
    Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    Label->SetWorldSize(10.0f);
    Label->SetRelativeLocation(FVector(-8.5f, 0.0f, 45.0f));
    Label->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
}

void AHumanityTrinityRebuildLightSwitch::BeginPlay()
{
    Super::BeginPlay();

    for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It)
    {
        LightingController = *It;
        break;
    }

    UpdateVisualState();
}

void AHumanityTrinityRebuildLightSwitch::Interact(AActor* Interactor)
{
    if (!LightingController)
    {
        for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It)
        {
            LightingController = *It;
            break;
        }
    }

    if (LightingController)
    {
        LightingController->ToggleMaster();
        UpdateVisualState();
    }
}

FString AHumanityTrinityRebuildLightSwitch::GetInteractionPrompt() const
{
    if (LightingController && LightingController->AreMainLightsOn())
    {
        return TEXT("E - Turn OFF classroom lights");
    }
    return TEXT("E - Turn ON classroom lights");
}

void AHumanityTrinityRebuildLightSwitch::UpdateVisualState()
{
    const bool bLightsOn = !LightingController || LightingController->AreMainLightsOn();
    Paddle->SetRelativeRotation(FRotator(0.0f, bLightsOn ? -14.0f : 14.0f, 0.0f));

    if (UMaterialInstanceDynamic* PlateMaterial = BackPlate->CreateAndSetMaterialInstanceDynamic(0))
    {
        PlateMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f, 0.14f, 0.17f));
    }
    if (UMaterialInstanceDynamic* PaddleMaterial = Paddle->CreateAndSetMaterialInstanceDynamic(0))
    {
        PaddleMaterial->SetVectorParameterValue(
            TEXT("Color"),
            bLightsOn ? FLinearColor(0.85f, 0.70f, 0.20f) : FLinearColor(0.28f, 0.32f, 0.38f));
    }
}
