#include "HumanityTrinityRebuildPlayerCharacter.h"

#include "HumanityTrinityRebuildLightSwitch.h"
#include "HumanityTrinityRebuildHideAndSeekGameMode.h"
#include "HumanityTrinityRebuildHider.h"
#include "HumanityTrinityRebuildRoomInteraction.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "Materials/MaterialInterface.h"

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
    if (bHideAndSeekMode) UpdateFootsteps(DeltaSeconds);
    if (bHideAndSeekMode && bTouchHeld)
    {
        TouchPulseSeconds -= DeltaSeconds;
        if (TouchPulseSeconds <= 0.0f)
        {
            TouchPulseSeconds = 0.45f;
            PerformTouch();
        }
    }
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
    PlayerInputComponent->BindAction(TEXT("TouchNearby"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::StartTouch);
    PlayerInputComponent->BindAction(TEXT("TouchNearby"), IE_Released, this, &AHumanityTrinityRebuildPlayerCharacter::StopTouch);
    PlayerInputComponent->BindAction(TEXT("RestartHideAndSeek"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::RestartHideAndSeek);
    PlayerInputComponent->BindAction(TEXT("ReturnToModeMenu"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::ReturnToModeMenu);
    PlayerInputComponent->BindAction(TEXT("GameCrouch"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::StartCrouch);
    PlayerInputComponent->BindAction(TEXT("GameCrouch"), IE_Released, this, &AHumanityTrinityRebuildPlayerCharacter::StopCrouch);
    PlayerInputComponent->BindAction(TEXT("PracticeMode"), IE_Pressed, this, &AHumanityTrinityRebuildPlayerCharacter::TogglePractice);
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
    if (bHideAndSeekMode)
    {
        if (auto* Game = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildHideAndSeekGameMode>())
            Game->ReadyToHide();
        return;
    }
    Jump();
}

void AHumanityTrinityRebuildPlayerCharacter::StopJump()
{
    StopJumping();
}

void AHumanityTrinityRebuildPlayerCharacter::Interact()
{
    UpdateFocusedInteractable();
    if (bHideAndSeekMode)
    {
        // In darkness the distant line-of-sight prompts are not a sense.
        // Only a door already within arm's reach can be operated.
        if (RoomInteraction && FocusedRoomComponent &&
            RoomInteraction->GetPropDoorIndex(FocusedRoomComponent) >= 0)
        {
            RoomInteraction->Interact(FocusedRoomComponent);
        }
        return;
    }
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
    bSlowWalkHeld = true;
    UpdateWalkSpeed();
}

void AHumanityTrinityRebuildPlayerCharacter::StopSlowWalk()
{
    bSlowWalkHeld = false;
    UpdateWalkSpeed();
}

void AHumanityTrinityRebuildPlayerCharacter::StartTouch()
{
    if (!bHideAndSeekMode) return;
    bTouchHeld = true;
    TouchPulseSeconds = 0.45f;
    UpdateWalkSpeed();
    PerformTouch();
}

void AHumanityTrinityRebuildPlayerCharacter::StopTouch()
{
    bTouchHeld = false;
    UpdateWalkSpeed();
}

void AHumanityTrinityRebuildPlayerCharacter::UpdateWalkSpeed()
{
    GetCharacterMovement()->MaxWalkSpeed = bHideAndSeekMode && bTouchHeld
        ? 80.0f : (bSlowWalkHeld ? 120.0f : 240.0f);
    GetCharacterMovement()->MaxWalkSpeedCrouched = bTouchHeld ? 55.0f : 65.0f;
}

void AHumanityTrinityRebuildPlayerCharacter::StartCrouch()
{
    if (bHideAndSeekMode) Crouch();
}
void AHumanityTrinityRebuildPlayerCharacter::StopCrouch() { UnCrouch(); }
void AHumanityTrinityRebuildPlayerCharacter::TogglePractice()
{
    if (auto* Game = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildHideAndSeekGameMode>()) Game->TogglePracticeMode();
}
float AHumanityTrinityRebuildPlayerCharacter::GetFootstepLoudness() const
{
    return bIsCrouched ? .09f : (bSlowWalkHeld || bTouchHeld ? .18f : .85f);
}
FString AHumanityTrinityRebuildPlayerCharacter::GetMovementHint() const
{
    if (GetVelocity().Size2D() < 3) return TEXT("Still / silent");
    return bIsCrouched ? TEXT("Crouching / very quiet") :
        (bSlowWalkHeld || bTouchHeld ? TEXT("Careful steps / quiet") : TEXT("Walking / audible"));
}
void AHumanityTrinityRebuildPlayerCharacter::UpdateFootsteps(float Dt)
{
    FVector Camera = FirstPersonCamera->GetRelativeLocation();
    Camera.Z = FMath::FInterpTo(Camera.Z, bIsCrouched ? 35.f : 66.f, Dt, 10);
    FirstPersonCamera->SetRelativeLocation(Camera);
    const float Distance = FVector::Dist2D(GetActorLocation(), PreviousFootPosition);
    PreviousFootPosition = GetActorLocation();
    auto* Game = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildHideAndSeekGameMode>();
    // Teleports/restarts and airborne movement do not manufacture footsteps.
    if (!Game || !Game->IsRoundRunning() || Distance > 80 || !GetCharacterMovement()->IsMovingOnGround())
    { FootDistance=0; return; }
    FootDistance += Distance;
    const float Stride = bIsCrouched ? 32.f : 52.f;
    if (FootDistance >= Stride)
    {
        FootDistance = FMath::Fmod(FootDistance, Stride);
        const float Loudness = GetFootstepLoudness();
        Game->ReportPlayerNoise(GetActorLocation(), Loudness);
        if (PlayerFootstep) UGameplayStatics::PlaySound2D(this, PlayerFootstep, Loudness*.22f, .78f);
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleCurtains()
{
    if (bHideAndSeekMode) return;
    if (AHumanityTrinityRebuildRoomInteraction* Interaction = FindRoomInteraction())
    {
        Interaction->ToggleCurtains();
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleScreen()
{
    if (bHideAndSeekMode) return;
    if (AHumanityTrinityRebuildRoomInteraction* Interaction = FindRoomInteraction())
    {
        Interaction->ToggleScreen();
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleMasterLights()
{
    if (bHideAndSeekMode) return;
    if (AHumanityTrinityRebuildLightingController* LightController = FindLightingController())
    {
        LightController->ToggleMaster();
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleFrontZone()
{
    if (bHideAndSeekMode) return;
    if (AHumanityTrinityRebuildLightingController* LightController = FindLightingController())
    {
        LightController->ToggleZone(EHumanityTrinityRebuildLightZone::Front);
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleMiddleZone()
{
    if (bHideAndSeekMode) return;
    if (AHumanityTrinityRebuildLightingController* LightController = FindLightingController())
    {
        LightController->ToggleZone(EHumanityTrinityRebuildLightZone::Middle);
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleRearZone()
{
    if (bHideAndSeekMode) return;
    if (AHumanityTrinityRebuildLightingController* LightController = FindLightingController())
    {
        LightController->ToggleZone(EHumanityTrinityRebuildLightZone::Rear);
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ToggleStageZone()
{
    if (bHideAndSeekMode) return;
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
    if (bHideAndSeekMode) return FString();
    if (FocusedSwitch)
    {
        return FocusedSwitch->GetInteractionPrompt();
    }
    return RoomInteraction && FocusedRoomComponent
        ? RoomInteraction->GetInteractionPrompt(FocusedRoomComponent) : FString();
}

void AHumanityTrinityRebuildPlayerCharacter::EnableHideAndSeekMode()
{
    bHideAndSeekMode = true;
    bTouchHeld = false;
    TouchPulseSeconds = 0.0f;
    UpdateWalkSpeed();
    // The chair rows have passages narrower than the walkthrough capsule.
    // A 48 cm body width represents moving sideways between furniture.
    GetCapsuleComponent()->SetCapsuleSize(24.0f, 92.0f, true);
    // The NPC must touch the player's physical standing/crouched body to catch them.
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    GetCharacterMovement()->SetCrouchedHalfHeight(60.0f);
    UnCrouch();
    PreviousFootPosition = GetActorLocation();
    FootDistance = 0;
    PlayerFootstep = LoadObject<USoundWave>(nullptr,
        TEXT("/Game/HumanityTrinityRebuild/Audio/SW_HiderFootstep.SW_HiderFootstep"));
    InteractionDistanceCm = 95.0f;
    TouchMessage.Empty();
    TouchMessageUntil = 0.0f;
}

FString AHumanityTrinityRebuildPlayerCharacter::GetCurrentTouchMessage() const
{
    return bHideAndSeekMode && GetWorld() && GetWorld()->GetTimeSeconds() < TouchMessageUntil
        ? TouchMessage : FString();
}

void AHumanityTrinityRebuildPlayerCharacter::PerformTouch()
{
    if (!bHideAndSeekMode) return;
    AHumanityTrinityRebuildHideAndSeekGameMode* Game =
        GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildHideAndSeekGameMode>();
    if (!Game || (!Game->IsRoundRunning() && (!Game->IsPlayerHiding() || Game->IsRoundFinished()))) return;

    const FVector Start = FirstPersonCamera->GetComponentLocation() - FVector(0.0f, 0.0f, 45.0f);
    FVector Forward = GetControlRotation().Vector();
    Forward.Normalize();
    const FVector End = Start + Forward * 105.0f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HideAndSeekHandTouch), true, this);
    Params.bReturnFaceIndex = true;
    const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity,
        ECC_Visibility, FCollisionShape::MakeSphere(12.0f), Params);
    if (!bHit)
    {
        TouchMessage = TEXT("Your hand finds only air");
    }
    else if (Cast<AHumanityTrinityRebuildHider>(Hit.GetActor()))
    {
        TouchMessage = TEXT("You touched a person!");
        Game->TryCatchHider(Hit.GetActor());
    }
    else if (Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("HidingAreaBoundary")))
    {
        TouchMessage = TEXT("The play-area rail / hide on the classroom side");
    }
    else
    {
        int32 Section = 0;
        UMaterialInterface* Material = Hit.GetComponent()
            ? Hit.GetComponent()->GetMaterialFromCollisionFaceIndex(Hit.FaceIndex, Section) : nullptr;
        const FString Name = Material ? Material->GetName().ToLower() : FString();
        FString Surface = TEXT("A firm surface");
        if (Name.Contains(TEXT("wood")) || Name.Contains(TEXT("oak")) || Name.Contains(TEXT("slat"))) Surface = TEXT("Hard wood / a smooth grain");
        else if (Name.Contains(TEXT("metal")) || Name.Contains(TEXT("steel")) || Name.Contains(TEXT("brass"))) Surface = TEXT("Cool metal / a rigid edge");
        else if (Name.Contains(TEXT("fabric")) || Name.Contains(TEXT("curtain"))) Surface = TEXT("Soft fabric / folds under your hand");
        else if (Name.Contains(TEXT("plastic")) || Name.Contains(TEXT("shell"))) Surface = TEXT("Smooth plastic / a curved surface");
        else if (Name.Contains(TEXT("floor"))) Surface = TEXT("A firm floor / fine surface texture");
        else if (Name.Contains(TEXT("table"))) Surface = TEXT("Smooth tabletop / a firm edge");
        else if (Name.Contains(TEXT("wall")) || Name.Contains(TEXT("plaster"))) Surface = TEXT("A flat wall / slightly rough");
        const TCHAR* Height = Hit.ImpactPoint.Z < 45 ? TEXT("low") : (Hit.ImpactPoint.Z < 95 ? TEXT("waist height") : TEXT("high"));
        TouchMessage = FString::Printf(TEXT("%s | %s | %.0f cm from your hand"), *Surface, Height, Hit.Distance);
    }
    TouchMessageUntil = GetWorld()->GetTimeSeconds() + 2.4f;
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] TOUCH result=%s hit=%s distance=%.1f"),
        *TouchMessage, bHit && Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("none"),
        bHit ? Hit.Distance : -1.0f);
}

void AHumanityTrinityRebuildPlayerCharacter::RestartHideAndSeek()
{
    if (!bHideAndSeekMode) return;
    if (AHumanityTrinityRebuildHideAndSeekGameMode* Game =
        GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildHideAndSeekGameMode>())
    {
        Game->RestartRound();
    }
}

void AHumanityTrinityRebuildPlayerCharacter::ReturnToModeMenu()
{
    if (bHideAndSeekMode)
    {
        UGameplayStatics::OpenLevel(this, TEXT("/Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildModeMenu"));
    }
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
