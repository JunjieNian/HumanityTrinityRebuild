#include "HumanityTrinityRebuildModeMenuPlayerController.h"

#include "HumanityTrinityRebuildModeMenuGameMode.h"
#include "HumanityTrinityRebuildModeMenuHUD.h"
#include "InputCoreTypes.h"

void AHumanityTrinityRebuildModeMenuPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::One, IE_Pressed, this,
                            &AHumanityTrinityRebuildModeMenuPlayerController::ChooseWalkthrough);
    InputComponent->BindKey(EKeys::Two, IE_Pressed, this,
                            &AHumanityTrinityRebuildModeMenuPlayerController::ChooseHideAndSeek);
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this,
                            &AHumanityTrinityRebuildModeMenuPlayerController::ClickMenu);
    InputComponent->BindKey(EKeys::M, IE_Pressed, this,
                            &AHumanityTrinityRebuildModeMenuPlayerController::ReturnToModeMenu);
}

void AHumanityTrinityRebuildModeMenuPlayerController::ChooseWalkthrough()
{
    if (auto* Menu = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildModeMenuGameMode>())
        Menu->EnterWalkthrough();
}

void AHumanityTrinityRebuildModeMenuPlayerController::ChooseHideAndSeek()
{
    if (auto* Menu = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildModeMenuGameMode>())
        Menu->EnterHideAndSeek();
}

void AHumanityTrinityRebuildModeMenuPlayerController::ClickMenu()
{
    auto* Menu = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildModeMenuGameMode>();
    auto* MenuHUD = Cast<AHumanityTrinityRebuildModeMenuHUD>(GetHUD());
    if (!Menu || !Menu->IsChoosingMode() || !MenuHUD)
        return;
    float X = 0, Y = 0;
    if (GetMousePosition(X, Y))
        MenuHUD->SelectAt(X, Y);
}

void AHumanityTrinityRebuildModeMenuPlayerController::ReturnToModeMenu()
{
    if (auto* Menu = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildModeMenuGameMode>())
        if (!Menu->IsChoosingMode())
            Menu->ReturnToMenu();
}
