#include "HumanityTrinityRebuildHUD.h"

#include "HumanityTrinityRebuildPlayerCharacter.h"
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
    DrawRect(FLinearColor(0.92f, 0.95f, 1.0f, 0.7f), CenterX - 1.0f, CenterY - 1.0f, 2.0f, 2.0f);

    const AHumanityTrinityRebuildPlayerCharacter* Player = Cast<AHumanityTrinityRebuildPlayerCharacter>(GetOwningPawn());
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

    DrawText(
        TEXT("WASD Move   Shift Slow   E Use   L Lights   1-4 Zones   C Curtains   P Display   Esc Exit"),
        FLinearColor(0.78f, 0.82f, 0.88f, 0.9f),
        24.0f,
        Canvas->ClipY - 34.0f,
        GEngine->GetSmallFont(),
        1.0f,
        false);
}
