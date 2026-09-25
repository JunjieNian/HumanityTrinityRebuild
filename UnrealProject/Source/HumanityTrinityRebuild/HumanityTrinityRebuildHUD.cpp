#include "HumanityTrinityRebuildHUD.h"

#include "HumanityTrinityRebuildPlayerCharacter.h"
#include "HumanityTrinityRebuildHideAndSeekGameMode.h"
#include "HumanityTrinityRebuildModeMenuGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AHumanityTrinityRebuildHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas || !GEngine)
    {
        return;
    }

    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;
    const AHumanityTrinityRebuildPlayerCharacter* Player = Cast<AHumanityTrinityRebuildPlayerCharacter>(GetOwningPawn());
    if (Player && Player->IsHideAndSeekMode())
    {
        const AHumanityTrinityRebuildHideAndSeekGameMode* Game =
            GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildHideAndSeekGameMode>();
        if (Game && Game->IsRoundRunning() && !Game->IsPracticeMode())
        {
            // The room's baked/indirect light can survive switching dynamic
            // fixtures off. This rule makes the played round truly sightless.
            DrawRect(FLinearColor::Black, 0.0f, 0.0f, Canvas->ClipX, Canvas->ClipY);
        }
        DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.60f), 0.0f, 0.0f, Canvas->ClipX, 56.0f);
        DrawText(Game ? Game->GetStatusLine() : TEXT("HIDE AND SEEK"),
            FLinearColor(1.0f, 0.91f, 0.70f), 24.0f, 17.0f,
            GEngine->GetMediumFont(), 1.0f, false);
        const FString Touch = Player->GetCurrentTouchMessage();
        if (!Touch.IsEmpty())
        {
            float Width = 0.0f, Height = 0.0f;
            GetTextSize(Touch, Width, Height, GEngine->GetMediumFont(), 1.0f);
            DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f),
                CenterX - Width * 0.5f - 14.0f, CenterY + 40.0f, Width + 28.0f, Height + 16.0f);
            DrawText(Touch, FLinearColor::White, CenterX - Width * 0.5f,
                CenterY + 48.0f, GEngine->GetMediumFont(), 1.0f, false);
        }
        const FString PracticeHint = Game ? Game->GetPracticeHint() : FString();
        float HintWidth = 0, HintHeight = 0;
        GetTextSize(PracticeHint, HintWidth, HintHeight, GEngine->GetSmallFont());
        DrawRect(FLinearColor(0,0,0,.6f),16,68,FMath::Max(245.f, HintWidth + 16),PracticeHint.IsEmpty()?28:53);
        DrawText(Player->GetMovementHint(), FLinearColor(.75f,.9f,.88f),24,74,GEngine->GetSmallFont());
        if (!PracticeHint.IsEmpty())
            DrawText(PracticeHint,FLinearColor(.9f,.82f,.64f),24,96,GEngine->GetSmallFont());
        DrawRect(FLinearColor(0,0,0,.65f),0,Canvas->ClipY-44,Canvas->ClipX,44);
        DrawText(Game && Game->IsPlayerHiding()
            ? TEXT("WASD Move | Shift Quiet | Ctrl Crouch | Hold F Feel | Space Ready | Tab Practice | R Restart | M Menu")
            : TEXT("WASD Move | Shift Quiet | Ctrl Crouch | Hold F Feel | E Door | Tab Practice | R Restart | M Menu"),
            FLinearColor(0.85f, 0.85f, 0.85f, 0.9f), 24.0f,
            Canvas->ClipY - 34.0f, GEngine->GetSmallFont(), 1.0f, false);
        return;
    }
    DrawRect(FLinearColor(0.92f, 0.95f, 1.0f, 0.7f), CenterX - 1.0f, CenterY - 1.0f, 2.0f, 2.0f);

    if (Player)
    {
        const FString Prompt = Player->GetCurrentInteractionPrompt();
        if (!Prompt.IsEmpty())
        {
            float TextWidth = 0.0f;
            float TextHeight = 0.0f;
            GetTextSize(Prompt, TextWidth, TextHeight, GEngine->GetMediumFont(), 0.9f);
            DrawRect(FLinearColor(0.025f, 0.035f, 0.04f, 0.70f), CenterX - TextWidth * 0.5f - 12.0f,
                CenterY + 24.0f, TextWidth + 24.0f, TextHeight + 12.0f);
            DrawText(Prompt, FLinearColor(1.0f, 0.89f, 0.55f, 1.0f), CenterX - TextWidth * 0.5f,
                CenterY + 30.0f, GEngine->GetMediumFont(), 0.9f, false);
        }
    }

    const bool bFromMenu = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildModeMenuGameMode>() != nullptr;
    DrawText(
        bFromMenu ? TEXT("WASD Move   Shift Slow   E Use   L Lights   1-4 Zones   C Curtains   P Display   M Menu   Esc Exit")
                  : TEXT("WASD Move   Shift Slow   E Use   L Lights   1-4 Zones   C Curtains   P Display   Esc Exit"),
        FLinearColor(0.78f, 0.82f, 0.88f, 0.9f),
        24.0f,
        Canvas->ClipY - 34.0f,
        GEngine->GetSmallFont(),
        1.0f,
        false);
}
