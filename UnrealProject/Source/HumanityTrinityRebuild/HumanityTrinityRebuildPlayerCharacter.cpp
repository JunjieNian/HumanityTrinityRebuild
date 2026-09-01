#include "HumanityTrinityRebuildPlayerCharacter.h"

#include "HumanityTrinityRebuildLightSwitch.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

AHumanityTrinityRebuildPlayerCharacter::AHumanityTrinityRebuildPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(34.0f, 92.0f);

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 66.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;
    FirstPersonCamera->PostProcessBlendWeight = 1.0f;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxWalkSpeed = 300.0f;
    GetCharacterMovement()->JumpZVelocity = 420.0f;
    GetCharacterMovement()->AirControl = 0.2f;
}

void AHumanityTrinityRebuildPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    CurrentExposure = LightAdaptedExposure;

    // The runtime map is intentionally minimal. This puts the observer just inside the front teaching end.
    if (GetActorLocation().Z < 50.0f)
    {
        SetActorLocation(FVector(0.0f, -220.0f, 96.0f));
    }

    if (AController* CurrentController = GetController())
    {
        CurrentController->SetControlRotation(FRotator(0.0f, -90.0f, 0.0f));
    }

    LightingController = FindLightingController();

    FPostProcessSettings& Settings = FirstPersonCamera->PostProcessSettings;
    Settings.bOverride_AutoExposureMethod = 1;
    Settings.AutoExposureMethod = AEM_Manual;
    Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = 1;
    Settings.AutoExposureApplyPhysicalCameraExposure = 0;
    Settings.bOverride_AutoExposureBias = 1;
    Settings.AutoExposureBias = CurrentExposure;

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            2000,
            12.0f,
            FColor::Cyan,
            TEXT("WASD move | Mouse look | E use switch | L master lights | 1-4 zones"));
    }
}

void AHumanityTrinityRebuildPlayerCharacter::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateFocusedInteractable();
    UpdateEyeAdaptation(DeltaSeconds);
}

void AHumanityTrinityRebuildPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AHumanityTrinityRebuildPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AHumanityTrinityRebuildPlayerCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AHumanityTrinityRebuildPlayerCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AHumanityTrinityRebuildPlayerCharacter::LookUp);

    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::StartJump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AHumanityTrinityRebuildPlayerCharacter::StopJump);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::Interact);
    PlayerInputComponent->BindAction(TEXT("ToggleMasterLights"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::ToggleMasterLights);
    PlayerInputComponent->BindAction(TEXT("ToggleFrontZone"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::ToggleFrontZone);
    PlayerInputComponent->BindAction(TEXT("ToggleMiddleZone"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::ToggleMiddleZone);
    PlayerInputComponent->BindAction(TEXT("ToggleRearZone"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::ToggleRearZone);
    PlayerInputComponent->BindAction(TEXT("ToggleStageZone"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::ToggleStageZone);
    PlayerInputComponent->BindAction(TEXT("QuitPrototype"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::QuitPrototype);
}

void AHumanityTrinityRebuildPlayerCharacter::MoveForward(const float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorForwardVector(), Value);
    }
}

void AHumanityTrinityRebuildPlayerCharacter::MoveRight(const float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        AddMovementInput(GetActorRightVector(), Value);
    }
}

void AHumanityTrinityRebuildPlayerCharacter::Turn(const float Value)
{
    AddControllerYawInput(Value);
}

void AHumanityTrinityRebuildPlayerCharacter::LookUp(const float Value)
{
    AddControllerPitchInput(Value);
}

void AHumanityTrinityRebuildPlayerCharacter::StartJump()
{
    Jump();
}

void AHumanityTrinityRebuildPlayerCharacter::StopJump()
{
    StopJumping();
}

void AHumanityTrinityRebuildPlayerCharacter::Interact()
{
    if (FocusedSwitch)
    {
        FocusedSwitch->Interact(this);
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleMasterLights()
{
    if (AHumanityTrinityRebuildLightingController* LightController = FindLightingController())
    {
        LightController->ToggleMaster();
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleFrontZone()
{
    if (AHumanityTrinityRebuildLightingController* LightController = FindLightingController())
    {
        LightController->ToggleZone(EHumanityTrinityRebuildLightZone::Front);
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleMiddleZone()
{
    if (AHumanityTrinityRebuildLightingController* LightController = FindLightingController())
    {
        LightController->ToggleZone(EHumanityTrinityRebuildLightZone::Middle);
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleRearZone()
{
    if (AHumanityTrinityRebuildLightingController* LightController = FindLightingController())
    {
        LightController->ToggleZone(EHumanityTrinityRebuildLightZone::Rear);
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleStageZone()
{
    if (AHumanityTrinityRebuildLightingController* LightController = FindLightingController())
    {
        LightController->ToggleZone(EHumanityTrinityRebuildLightZone::Stage);
    }
}

void AHumanityTrinityRebuildPlayerCharacter::QuitPrototype()
{
    UKismetSystemLibrary::QuitGame(
        this,
        Cast<APlayerController>(GetController()),
        EQuitPreference::Quit,
        false);
}

FString AHumanityTrinityRebuildPlayerCharacter::GetCurrentInteractionPrompt() const
{
    return FocusedSwitch ? FocusedSwitch->GetInteractionPrompt() : FString();
}

void AHumanityTrinityRebuildPlayerCharacter::UpdateFocusedInteractable()
{
    FocusedSwitch = nullptr;

    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * InteractionDistanceCm;
    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HumanityTrinityRebuildInteractionTrace), false, this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
    {
        FocusedSwitch = Cast<AHumanityTrinityRebuildLightSwitch>(Hit.GetActor());
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            2001,
            0.05f,
            FColor::Yellow,
            FocusedSwitch ? FocusedSwitch->GetInteractionPrompt() : TEXT(""));
    }
}

void AHumanityTrinityRebuildPlayerCharacter::UpdateEyeAdaptation(const float DeltaSeconds)
{
    AHumanityTrinityRebuildLightingController* LightController = FindLightingController();
    const bool bLightsOn = !LightController || LightController->AreMainLightsOn();
    const float TargetExposure = bLightsOn ? LightAdaptedExposure : DarkAdaptedExposure;
    const float AdaptationSpeed = bLightsOn ? BrightAdaptationSpeed : DarkAdaptationSpeed;

    CurrentExposure = FMath::FInterpTo(CurrentExposure, TargetExposure, DeltaSeconds, AdaptationSpeed);
    FirstPersonCamera->PostProcessSettings.AutoExposureBias = CurrentExposure;
}

AHumanityTrinityRebuildLightingController* AHumanityTrinityRebuildPlayerCharacter::FindLightingController()
{
    if (LightingController)
    {
        return LightingController;
    }

    for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It)
    {
        LightingController = *It;
        break;
    }

    return LightingController;
}
