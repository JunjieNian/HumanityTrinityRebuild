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
    DrawText(TEXT("+"), FLinearColor(0.92f, 0.95f, 1.0f, 0.9f), CenterX - 4.0f, CenterY - 9.0f, GEngine->GetSmallFont(), 1.0f, false);

    const AHumanityTrinityRebuildPlayerCharacter* Player = Cast<AHumanityTrinityRebuildPlayerCharacter>(GetOwningPawn());
    if (Player)
    {
        const FString Prompt = Player->GetCurrentInteractionPrompt();
        if (!Prompt.IsEmpty())
        {
            DrawText(Prompt, FLinearColor(1.0f, 0.82f, 0.25f, 1.0f), CenterX - 105.0f, CenterY + 28.0f, GEngine->GetMediumFont(), 0.9f, false);
        }
    }

    DrawText(
        TEXT("WASD Move   Mouse Look   E Use   L Lights   1-4 Zones   Esc Exit"),
        FLinearColor(0.78f, 0.82f, 0.88f, 0.9f),
        24.0f,
        Canvas->ClipY - 34.0f,
        GEngine->GetSmallFont(),
        1.0f,
        false);
}
