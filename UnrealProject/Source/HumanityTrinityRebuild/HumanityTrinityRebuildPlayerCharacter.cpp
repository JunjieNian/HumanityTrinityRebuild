#include "HumanityTrinityRebuildPlayerCharacter.h"

#include "HumanityTrinityRebuildLightSwitch.h"
#include "HumanityTrinityRebuildRoomInteraction.h"
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
    GetCharacterMovement()->MaxWalkSpeed = 240.0f;
    GetCharacterMovement()->MaxStepHeight = 25.0f;
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
    Settings.bOverride_WhiteTemp = 1;
    Settings.WhiteTemp = 4500.0f;

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
    PlayerInputComponent->BindAction(TEXT("SlowWalk"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::StartSlowWalk);
    PlayerInputComponent->BindAction(TEXT("SlowWalk"), IE_Released, this, &AHumanityTrinityRebuildPlayerCharacter::StopSlowWalk);
    PlayerInputComponent->BindAction(TEXT("ToggleCurtains"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::ToggleCurtains);
    PlayerInputComponent->BindAction(TEXT("ToggleScreen"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::ToggleScreen);
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
    UpdateFocusedInteractable();
    if (FocusedSwitch)
    {
        FocusedSwitch->Interact(this);
    }
    else if (RoomInteraction && FocusedRoomComponent)
    {
        RoomInteraction->Interact(FocusedRoomComponent);
    }
}

void AHumanityTrinityRebuildPlayerCharacter::StartSlowWalk()
{
    GetCharacterMovement()->MaxWalkSpeed = 120.0f;
}

void AHumanityTrinityRebuildPlayerCharacter::StopSlowWalk()
{
    GetCharacterMovement()->MaxWalkSpeed = 240.0f;
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleCurtains()
{
    if (AHumanityTrinityRebuildRoomInteraction* Interaction = FindRoomInteraction())
    {
        Interaction->ToggleCurtains();
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleScreen()
{
    if (AHumanityTrinityRebuildRoomInteraction* Interaction = FindRoomInteraction())
    {
        Interaction->ToggleScreen();
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
    if (FocusedSwitch)
    {
        return FocusedSwitch->GetInteractionPrompt();
    }
    return RoomInteraction && FocusedRoomComponent
        ? RoomInteraction->GetInteractionPrompt(FocusedRoomComponent) : FString();
}

void AHumanityTrinityRebuildPlayerCharacter::UpdateFocusedInteractable()
{
    FocusedSwitch = nullptr;
    FocusedRoomComponent = nullptr;

    const FVector Start = FirstPersonCamera->GetComponentLocation();
    const FVector End = Start + FirstPersonCamera->GetForwardVector() * InteractionDistanceCm;
    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HumanityTrinityRebuildInteractionTrace), false, this);

    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
    {
        FocusedSwitch = Cast<AHumanityTrinityRebuildLightSwitch>(Hit.GetActor());
        if (AHumanityTrinityRebuildRoomInteraction* Interaction = Cast<AHumanityTrinityRebuildRoomInteraction>(Hit.GetActor()))
        {
            RoomInteraction = Interaction;
            FocusedRoomComponent = Hit.GetComponent();
        }
    }
}

void AHumanityTrinityRebuildPlayerCharacter::UpdateEyeAdaptation(const float DeltaSeconds)
{
    AHumanityTrinityRebuildLightingController* LightController = FindLightingController();
    const bool bLightsOn = !LightController || LightController->AreMainLightsOn();
    AHumanityTrinityRebuildRoomInteraction* Interaction = FindRoomInteraction();
    const bool bScreenOn = Interaction && Interaction->IsScreenIlluminating();
    const float TargetExposure = bLightsOn ? LightAdaptedExposure : (bScreenOn ? -1.4f : DarkAdaptedExposure);
    const float AdaptationSpeed = bLightsOn || bScreenOn ? BrightAdaptationSpeed : DarkAdaptationSpeed;

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

AHumanityTrinityRebuildRoomInteraction* AHumanityTrinityRebuildPlayerCharacter::FindRoomInteraction()
{
    if (!RoomInteraction)
    {
        for (TActorIterator<AHumanityTrinityRebuildRoomInteraction> It(GetWorld()); It; ++It)
        {
            RoomInteraction = *It;
            break;
        }
    }
    return RoomInteraction;
}
