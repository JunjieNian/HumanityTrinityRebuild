#include "HumanityTrinityRebuildModeMenuHUD.h"

#include "HumanityTrinityRebuildModeMenuGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

FBox2D AHumanityTrinityRebuildModeMenuHUD::GetChoiceBox(int32 Choice) const
{
    if (!HasLayout())
        return FBox2D(EForceInit::ForceInit);
    const float Width = FMath::Clamp(ViewSize.X * 0.41f, 400.0f, 610.0f);
    const float Height = FMath::Clamp(ViewSize.Y * 0.115f, 74.0f, 105.0f);
    const float X = FMath::Max(45.0f, ViewSize.X * 0.095f);
    const float Y = ViewSize.Y * (Choice == 0 ? 0.39f : 0.54f);
    return FBox2D(FVector2D(X, Y), FVector2D(X + Width, Y + Height));
}

FVector2D AHumanityTrinityRebuildModeMenuHUD::GetChoiceCenter(int32 Choice) const
{
    return GetChoiceBox(Choice).GetCenter();
}

bool AHumanityTrinityRebuildModeMenuHUD::SelectAt(float X, float Y)
{
    auto* Menu = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildModeMenuGameMode>();
    if (!Menu || !Menu->IsChoosingMode() || !HasLayout())
        return false;
    if (GetChoiceBox(0).IsInside(FVector2D(X, Y)))
    {
        Menu->EnterWalkthrough();
        return true;
    }
    if (GetChoiceBox(1).IsInside(FVector2D(X, Y)))
    {
        Menu->EnterHideAndSeek();
        return true;
    }
    return false;
}

void AHumanityTrinityRebuildModeMenuHUD::DrawHUD()
{
    auto* Menu = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildModeMenuGameMode>();
    if (!Menu || !Menu->IsChoosingMode())
    {
        Super::DrawHUD();
        return;
    }
    if (!Canvas || !GEngine)
        return;

    const float W = Canvas->ClipX, H = Canvas->ClipY;
    ViewSize = FVector2D(W, H);
    const float Left = FMath::Max(45.0f, W * 0.095f);
    DrawRect(FLinearColor(0.015f, 0.025f, 0.03f, 0.33f), 0, 0, W, H);
    DrawRect(FLinearColor(0.015f, 0.025f, 0.03f, 0.88f), 0, 0, FMath::Min(W * 0.57f, 760.0f), H);
    DrawRect(FLinearColor(0.77f, 0.64f, 0.37f, 1), Left, H * 0.16f, 85, 4);
    DrawText(TEXT("TRINITY SPACE"), FLinearColor::White, Left, H * 0.19f, GEngine->GetLargeFont(), 1.45f, false);
    DrawText(TEXT("Explore the reconstruction or play in darkness"), FLinearColor(.78f, .83f, .81f), Left, H * 0.29f,
             GEngine->GetMediumFont(), 0.95f, false);

    float MouseX = -1, MouseY = -1;
    if (auto* Controller = GetOwningPlayerController())
        Controller->GetMousePosition(MouseX, MouseY);
    for (int32 Choice = 0; Choice < 2; ++Choice)
    {
        const FBox2D Box = GetChoiceBox(Choice);
        const bool bHover = Box.IsInside(FVector2D(MouseX, MouseY));
        DrawRect(bHover ? FLinearColor(.20f, .42f, .43f, .97f) : FLinearColor(.09f, .19f, .21f, .96f), Box.Min.X,
                 Box.Min.Y, Box.GetSize().X, Box.GetSize().Y);
        DrawRect(FLinearColor(.78f, .65f, .41f, 1), Box.Min.X, Box.Min.Y, 5, Box.GetSize().Y);
        DrawText(Choice == 0 ? TEXT("1   WALK THROUGH THE SPACE") : TEXT("2   PLAY HIDE AND SEEK"), FLinearColor::White,
                 Box.Min.X + 24, Box.Min.Y + 14, GEngine->GetMediumFont(), 1.15f, false);
        DrawText(Choice == 0 ? TEXT("Explore the lit room at your own pace")
                             : TEXT("Memorize the room, then search by sound and touch"),
                 FLinearColor(.79f, .87f, .84f), Box.Min.X + 24, Box.Min.Y + Box.GetSize().Y - 31,
                 GEngine->GetSmallFont(), 1.0f, false);
    }
    DrawText(TEXT("Click a mode or press 1 / 2    |    M returns to this menu"), FLinearColor(.83f, .78f, .67f), Left,
             H * .88f, GEngine->GetSmallFont(), 1.0f, false);
}
