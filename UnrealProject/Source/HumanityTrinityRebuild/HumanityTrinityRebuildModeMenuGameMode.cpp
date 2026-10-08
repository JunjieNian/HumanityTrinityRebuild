#include "HumanityTrinityRebuildModeMenuGameMode.h"

#include "HumanityTrinityRebuildModeMenuHUD.h"
#include "HumanityTrinityRebuildModeMenuPlayerController.h"
#include "HumanityTrinityRebuildPlayerCharacter.h"
#include "HumanityTrinityRebuildTeacherPatrol.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"

AHumanityTrinityRebuildModeMenuGameMode::AHumanityTrinityRebuildModeMenuGameMode()
{
    HUDClass = AHumanityTrinityRebuildModeMenuHUD::StaticClass();
    PlayerControllerClass = AHumanityTrinityRebuildModeMenuPlayerController::StaticClass();
}

void AHumanityTrinityRebuildModeMenuGameMode::BeginPlay()
{
    Super::BeginPlay();
    GetWorldTimerManager().SetTimerForNextTick(this, &AHumanityTrinityRebuildModeMenuGameMode::InitializeMenu);
}

void AHumanityTrinityRebuildModeMenuGameMode::InitializeMenu()
{
    APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    MenuPlayer = Controller ? Cast<AHumanityTrinityRebuildPlayerCharacter>(Controller->GetPawn()) : nullptr;
    if (!Controller || !MenuPlayer)
    {
        UE_LOG(LogTemp, Error, TEXT("[MODE_MENU] SETUP_FAIL missing controller or character"));
        return;
    }
    MenuPlayer->SetActorLocation(FVector(0, -220, 96), false, nullptr, ETeleportType::TeleportPhysics);
    Controller->SetControlRotation(FRotator(-5, -90, 0));
    ReturnToMenu();
    UE_LOG(LogTemp, Display, TEXT("[MODE_MENU] READY map=%s"), *GetWorld()->GetMapName());

    FString Choice;
    if (FParse::Value(FCommandLine::Get(), TEXT("ModeMenuSelfTest="), Choice))
    {
        FTimerHandle TestTimer;
        GetWorldTimerManager().SetTimer(TestTimer, this, &AHumanityTrinityRebuildModeMenuGameMode::RunMenuSelfTest,
                                        1.0f, false);
    }
}

void AHumanityTrinityRebuildModeMenuGameMode::ReturnToMenu()
{
    if (auto* Patrol = AHumanityTrinityRebuildTeacherPatrol::Find(GetWorld()))
    {
        if (Patrol->HasFailed()) return;
        Patrol->CancelPatrol();
    }
    APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    if (!Controller || !MenuPlayer)
        return;
    bChoosingMode = true;
    MenuPlayer->GetCharacterMovement()->StopMovementImmediately();
    MenuPlayer->DisableInput(Controller);
    Controller->SetIgnoreMoveInput(true);
    Controller->SetIgnoreLookInput(true);
    Controller->bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Controller->SetInputMode(InputMode);
}

void AHumanityTrinityRebuildModeMenuGameMode::ToggleTeacherPatrol()
{
    if (!bChoosingMode) return;
    const bool bEnabled = !AHumanityTrinityRebuildTeacherPatrol::IsSessionEnabled();
    AHumanityTrinityRebuildTeacherPatrol::SetSessionEnabled(bEnabled);
    if (auto* Patrol = AHumanityTrinityRebuildTeacherPatrol::Find(GetWorld()))
        Patrol->SetEnabled(bEnabled);
    UE_LOG(LogTemp, Display, TEXT("[MODE_MENU] TEACHER_PATROL enabled=%s"), bEnabled ? TEXT("YES") : TEXT("NO"));
}

bool AHumanityTrinityRebuildModeMenuGameMode::IsTeacherPatrolEnabled() const
{
    return AHumanityTrinityRebuildTeacherPatrol::IsSessionEnabled();
}

void AHumanityTrinityRebuildModeMenuGameMode::EnterWalkthrough()
{
    if (!bChoosingMode || !MenuPlayer)
        return;
    APlayerController* Controller = GetWorld()->GetFirstPlayerController();
    bChoosingMode = false;
    Controller->SetIgnoreMoveInput(false);
    Controller->SetIgnoreLookInput(false);
    Controller->bShowMouseCursor = false;
    Controller->SetInputMode(FInputModeGameOnly());
    MenuPlayer->EnableInput(Controller);
    UE_LOG(LogTemp, Display, TEXT("[MODE_MENU] SELECT walkthrough same_world=%s"), *GetWorld()->GetMapName());
}

void AHumanityTrinityRebuildModeMenuGameMode::EnterHideAndSeek()
{
    if (!bChoosingMode)
        return;
    bChoosingMode = false;
    UE_LOG(LogTemp, Display, TEXT("[MODE_MENU] SELECT hide_and_seek same_window=YES"));
    UGameplayStatics::OpenLevel(this, TEXT("/Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildHideAndSeek"));
}

void AHumanityTrinityRebuildModeMenuGameMode::EnterPlayerHiding()
{
    if (!bChoosingMode)
        return;
    bChoosingMode = false;
    UE_LOG(LogTemp, Display, TEXT("[MODE_MENU] SELECT player_hides same_window=YES"));
    UGameplayStatics::OpenLevel(this, TEXT("/Game/HumanityTrinityRebuild/Maps/L_HumanityTrinityRebuildHideAndSeek"),
                               true, TEXT("Role=Hider"));
}

void AHumanityTrinityRebuildModeMenuGameMode::RunMenuSelfTest()
{
    FString Choice;
    FParse::Value(FCommandLine::Get(), TEXT("ModeMenuSelfTest="), Choice);
    auto* Controller = GetWorld()->GetFirstPlayerController();
    auto* MenuHUD = Controller ? Cast<AHumanityTrinityRebuildModeMenuHUD>(Controller->GetHUD()) : nullptr;
    if (!MenuHUD || !MenuHUD->HasLayout() || !bChoosingMode || !Controller->bShowMouseCursor)
    {
        UE_LOG(LogTemp, Error, TEXT("[MODE_MENU_SELFTEST] FAIL menu_not_ready hud=%s layout=%s choosing=%s cursor=%s"),
               MenuHUD ? TEXT("YES") : TEXT("NO"), MenuHUD && MenuHUD->HasLayout() ? TEXT("YES") : TEXT("NO"),
               bChoosingMode ? TEXT("YES") : TEXT("NO"),
               Controller && Controller->bShowMouseCursor ? TEXT("YES") : TEXT("NO"));
        FPlatformMisc::RequestExit(false);
        return;
    }
    // Exercise the same hit target used by a mouse, while preserving the user's
    // session preference for the following mode-travel assertion.
    const bool bInitialPatrol = IsTeacherPatrolEnabled();
    const FVector2D ToggleCenter = MenuHUD->GetTeacherToggleCenter();
    const bool bFirstClick = MenuHUD->SelectAt(ToggleCenter.X, ToggleCenter.Y);
    const bool bToggled = IsTeacherPatrolEnabled() != bInitialPatrol;
    const bool bSecondClick = MenuHUD->SelectAt(ToggleCenter.X, ToggleCenter.Y);
    const bool bTogglePass = bFirstClick && bToggled && bSecondClick &&
        IsTeacherPatrolEnabled() == bInitialPatrol && bChoosingMode;
    UE_LOG(LogTemp, Display, TEXT("[MODE_MENU_SELFTEST] teacher_checkbox=%s preference_restored=%s"),
        bTogglePass ? TEXT("PASS") : TEXT("FAIL"),
        IsTeacherPatrolEnabled() == bInitialPatrol ? TEXT("YES") : TEXT("NO"));
    if (!bTogglePass)
    {
        FPlatformMisc::RequestExit(false);
        return;
    }
    const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), TEXT("ModeMenu.png"));
    FScreenshotRequest::RequestScreenshot(Path, false, false);
    UE_LOG(LogTemp, Display, TEXT("[MODE_MENU_SELFTEST] MENU_VISIBLE requested=%s"), *Path);
    FTimerHandle ChoiceTimer;
    GetWorldTimerManager().SetTimer(
        ChoiceTimer,
        [this, MenuHUD, Choice]() {
            const bool bHide = Choice.Equals(TEXT("hide"), ESearchCase::IgnoreCase);
            const bool bPlayerHide = Choice.Equals(TEXT("playerhide"), ESearchCase::IgnoreCase);
            const FVector2D Center = MenuHUD->GetChoiceCenter(bPlayerHide ? 2 : (bHide ? 1 : 0));
            MenuHUD->SelectAt(Center.X, Center.Y);
            if (!bHide && !bPlayerHide)
            {
                auto* Controller = GetWorld()->GetFirstPlayerController();
                const bool bPass = !bChoosingMode && Controller && !Controller->bShowMouseCursor;
                UE_LOG(LogTemp, Display, TEXT("[MODE_MENU_SELFTEST] walkthrough=%s"),
                       bPass ? TEXT("PASS") : TEXT("FAIL"));
                FTimerHandle ExitTimer;
                GetWorldTimerManager().SetTimer(ExitTimer, []() { FPlatformMisc::RequestExit(false); }, 0.5f, false);
            }
        },
        0.6f, false);
}
