#include "HumanityTrinityRebuildHUD.h"

#include "HumanityTrinityRebuildPlayerCharacter.h"
#include "HumanityTrinityRebuildHideAndSeekGameMode.h"
#include "HumanityTrinityRebuildModeMenuGameMode.h"
#include "HumanityTrinityRebuildTeacherPatrol.h"
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
    const auto* Patrol = AHumanityTrinityRebuildTeacherPatrol::Find(GetWorld());
    if (Patrol && Patrol->IsActive())
    {
        const bool bFailed = Patrol->HasFailed();
        const bool bWarning = Patrol->GetPhase() == ETeacherPatrolPhase::Warning;
        const bool bRecovering = Patrol->GetPhase() == ETeacherPatrolPhase::Recovering;
        const float Scale = FMath::Clamp(Canvas->ClipX / 1280.0f, .65f, 1.0f);
        const float Left = 24.0f;
        TArray<FString> StatusLines;
        Patrol->GetStatusLine().ParseIntoArrayLines(StatusLines, true);
        const float InstructionY = 55.f + 23.f * StatusLines.Num();
        const float BannerHeight = (InstructionY + 59.f) * Scale;
        const FLinearColor Accent = bFailed ? FLinearColor(1.f, .24f, .19f)
            : (bRecovering ? FLinearColor(.57f, .93f, .78f) : FLinearColor(1.f, .78f, .32f));
        if (bFailed) DrawRect(FLinearColor(.015f, .005f, .005f, .95f), 0, 0, Canvas->ClipX, Canvas->ClipY);
        DrawRect(FLinearColor(.02f, .025f, .03f, .91f), 0, 0, Canvas->ClipX, BannerHeight);
        DrawRect(Accent, 0, 0, 6, BannerHeight);
        const FString Headline = bFailed ? TEXT("DISCOVERED - GAME OVER")
            : (bWarning ? FString::Printf(TEXT("TEACHER APPROACHING  |  %d seconds to hide"),
                    FMath::Max(0, FMath::CeilToInt(Patrol->GetSecondsRemaining())))
                : (bRecovering ? TEXT("THE TEACHER HAS LEFT") : TEXT("TEACHER INSPECTION - STAY HIDDEN")));
        DrawText(Headline, Accent, Left, 14.f * Scale, GEngine->GetMediumFont(), 1.25f * Scale, false);
        for (int32 Line = 0; Line < StatusLines.Num(); ++Line)
            DrawText(StatusLines[Line], FLinearColor::White, Left, (48.f + 23.f * Line) * Scale,
                GEngine->GetSmallFont(), Scale, false);
        DrawText(bFailed ? TEXT("The teacher found activity in the classroom. The game will close automatically.")
            : (bRecovering ? TEXT("Open your side-room door with E, then return to the classroom to continue.")
                : TEXT("L: lights + display OFF   |   Move to either dark room beside the stage   |   E: shut its door")),
            FLinearColor(.88f, .91f, .92f), Left, InstructionY * Scale, GEngine->GetSmallFont(), Scale, false);
        DrawText(bFailed ? TEXT("This run has ended.")
            : (bRecovering ? TEXT("The normal round remains paused while everyone returns.")
                : TEXT("Everyone must be inside with the door closed. Wait until the teacher leaves.")),
            FLinearColor(.76f, .82f, .84f), Left, (InstructionY + 25.f) * Scale, GEngine->GetSmallFont(), Scale, false);
        if (!bFailed)
        {
            DrawRect(FLinearColor(1, 1, 1, .8f), CenterX - 1, CenterY - 1, 2, 2);
            const FString Prompt = Player ? Player->GetCurrentInteractionPrompt() : FString();
            if (!Prompt.IsEmpty())
            {
                float Width = 0, Height = 0;
                GetTextSize(Prompt, Width, Height, GEngine->GetMediumFont(), .9f * Scale);
                DrawRect(FLinearColor(0, 0, 0, .8f), CenterX - Width * .5f - 12, CenterY + 24, Width + 24, Height + 12);
                DrawText(Prompt, Accent, CenterX - Width * .5f, CenterY + 30,
                    GEngine->GetMediumFont(), .9f * Scale, false);
            }
            DrawRect(FLinearColor(0,0,0,.75f), 0, Canvas->ClipY - 44, Canvas->ClipX, 44);
            DrawText(TEXT("WASD Move   |   Shift Slow   |   L Emergency lights out   |   E Door   |   Round paused"),
                FLinearColor(.9f,.9f,.88f), Left, Canvas->ClipY - 32, GEngine->GetSmallFont(), Scale, false);
        }
        return;
    }
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
        DrawTeacherPatrolPreference();
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

    DrawText(
        TEXT("WASD Move   Shift Slow   E Use   L Lights   1-4 Zones   C Curtains   P Display   M Menu   Esc Exit"),
        FLinearColor(0.78f, 0.82f, 0.88f, 0.9f),
        24.0f,
        Canvas->ClipY - 34.0f,
        GEngine->GetSmallFont(),
        1.0f,
        false);
    DrawTeacherPatrolPreference();
}

void AHumanityTrinityRebuildHUD::DrawTeacherPatrolPreference()
{
    const auto* Patrol = AHumanityTrinityRebuildTeacherPatrol::Find(GetWorld());
    const bool bEnabled = Patrol && Patrol->IsEnabled();
    const FString Text = bEnabled ? TEXT("T  Teacher inspections: ON") : TEXT("T  Teacher inspections: OFF (opt in)");
    float Width = 0, Height = 0;
    GetTextSize(Text, Width, Height, GEngine->GetSmallFont(), .9f);
    const float Y = Canvas->ClipY - 79.f;
    DrawRect(FLinearColor(0,0,0,.68f), 16, Y - 7, Width + 20, Height + 14);
    DrawText(Text, bEnabled ? FLinearColor(1.f,.80f,.42f) : FLinearColor(.70f,.80f,.81f),
        24, Y, GEngine->GetSmallFont(), .9f, false);
}
