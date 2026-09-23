#include "HumanityTrinityRebuildModeMenuGameMode.h"

#include "HumanityTrinityRebuildModeMenuHUD.h"
#include "HumanityTrinityRebuildModeMenuPlayerController.h"
#include "HumanityTrinityRebuildPlayerCharacter.h"
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
    const FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), TEXT("ModeMenu.png"));
    FScreenshotRequest::RequestScreenshot(Path, false, false);
    UE_LOG(LogTemp, Display, TEXT("[MODE_MENU_SELFTEST] MENU_VISIBLE requested=%s"), *Path);
    FTimerHandle ChoiceTimer;
    GetWorldTimerManager().SetTimer(
        ChoiceTimer,
        [this, MenuHUD, Choice]() {
            const bool bHide = Choice.Equals(TEXT("hide"), ESearchCase::IgnoreCase);
            const FVector2D Center = MenuHUD->GetChoiceCenter(bHide ? 1 : 0);
            MenuHUD->SelectAt(Center.X, Center.Y);
            if (!bHide)
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
