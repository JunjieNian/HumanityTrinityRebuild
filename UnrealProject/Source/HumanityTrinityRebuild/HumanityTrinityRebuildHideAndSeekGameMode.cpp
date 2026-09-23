#include "HumanityTrinityRebuildHideAndSeekGameMode.h"

#include "HumanityTrinityRebuildHider.h"
#include "HumanityTrinityRebuildLightingController.h"
#include "HumanityTrinityRebuildPlayerCharacter.h"
#include "HumanityTrinityRebuildRoomInteraction.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AHumanityTrinityRebuildHideAndSeekGameMode::AHumanityTrinityRebuildHideAndSeekGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AHumanityTrinityRebuildHideAndSeekGameMode::BeginPlay()
{
    Super::BeginPlay();
    bSelfTest = FParse::Param(FCommandLine::Get(), TEXT("HideAndSeekSelfTest"));
    bCaptureGame = FParse::Param(FCommandLine::Get(), TEXT("HideAndSeekCapture"));
    bPracticeMode = FParse::Param(FCommandLine::Get(), TEXT("HideAndSeekPractice"));
    if (bSelfTest)
        FMath::RandInit(20260923);
    GetWorldTimerManager().SetTimerForNextTick(this, &AHumanityTrinityRebuildHideAndSeekGameMode::BeginPreparation);
}

void AHumanityTrinityRebuildHideAndSeekGameMode::BeginPreparation()
{
    Seeker = Cast<AHumanityTrinityRebuildPlayerCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
    if (!Seeker)
    {
        UE_LOG(LogTemp, Error, TEXT("[HIDE_AND_SEEK] SETUP_FAIL no seeker pawn"));
        return;
    }
    Seeker->EnableHideAndSeekMode();
    Seeker->GetCharacterMovement()->StopMovementImmediately();
    Seeker->SetActorLocation(FVector(0, -220, 96));
    Seeker->GetController()->SetControlRotation(FRotator(0, -90, 0));
    bRoundRunning = false;
    bRoundFinished = false;
    bSeekerWon = false;
    SecondsRemaining = 180.0f;
    if (Hider)
    {
        Hider->Destroy();
        Hider = nullptr;
    }
    for (TActorIterator<AHumanityTrinityRebuildRoomInteraction> It(GetWorld()); It; ++It)
    {
        It->SetScreenOn(false);
        It->SetCurtainsOpen(true);
        break;
    }
    for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It)
    {
        It->bResidualLightsEnabled = false;
        It->SetMasterLights(true);
        break;
    }
    PreparationEndsAt = GetWorld()->GetTimeSeconds() + (bSelfTest ? 1.0f : 12.0f);
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] PREPARATION seconds=%.0f"),
           PreparationEndsAt - GetWorld()->GetTimeSeconds());
    if (bSelfTest)
    {
        FTimerHandle CaptureTimer;
        GetWorldTimerManager().SetTimer(CaptureTimer, this,
                                        &AHumanityTrinityRebuildHideAndSeekGameMode::CaptureMemorize, 0.55f, false);
        GetWorldTimerManager().SetTimer(PreparationTimer, this, &AHumanityTrinityRebuildHideAndSeekGameMode::StartRound,
                                        1.0f, false);
    }
    else
    {
        GetWorldTimerManager().SetTimer(PreparationTimer, this, &AHumanityTrinityRebuildHideAndSeekGameMode::StartRound,
                                        12.0f, false);
    }
}

void AHumanityTrinityRebuildHideAndSeekGameMode::StartRound()
{
    if (!Seeker)
    {
        return;
    }
    for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It)
    {
        It->bResidualLightsEnabled = false;
        It->SetMasterLights(bPracticeMode);
        break;
    }
    const float Side = bSelfTest ? 1.0f : (FMath::RandBool() ? 1.0f : -1.0f);
    const FVector HideLocation(Side * 165.0f, -900.0f, 91.0f);
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Hider = GetWorld()->SpawnActor<AHumanityTrinityRebuildHider>(AHumanityTrinityRebuildHider::StaticClass(),
                                                                 HideLocation, FRotator::ZeroRotator, Spawn);
    if (!Hider)
    {
        UE_LOG(LogTemp, Error, TEXT("[HIDE_AND_SEEK] SETUP_FAIL hider spawn"));
        return;
    }
    Hider->SetSeeker(Seeker);
    bRoundRunning = true;
    SecondsRemaining = 180.0f;
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] ROUND_STARTED residual_lights=OFF hider=%s"),
           *Hider->GetActorLocation().ToString());
    if (bSelfTest)
    {
        FTimerHandle CaptureTimer;
        GetWorldTimerManager().SetTimer(CaptureTimer, this,
                                        &AHumanityTrinityRebuildHideAndSeekGameMode::CaptureBlackout, 0.30f, false);
        FTimerHandle TestTimer;
        GetWorldTimerManager().SetTimer(TestTimer, this, &AHumanityTrinityRebuildHideAndSeekGameMode::RunSelfTest,
                                        0.65f, false);
    }
}

void AHumanityTrinityRebuildHideAndSeekGameMode::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (SelfTestMoveUntil > GetWorld()->GetTimeSeconds() && Seeker)
        Seeker->AddMovementInput(FVector(1, 0, 0), 1.f);
    if (bRoundRunning)
    {
        SecondsRemaining = FMath::Max(0.0f, SecondsRemaining - DeltaSeconds);
        if (SecondsRemaining <= 0.0f)
        {
            FinishRound(false);
        }
    }
}

void AHumanityTrinityRebuildHideAndSeekGameMode::TryCatchHider(AActor* TouchedActor)
{
    if (bRoundRunning && Hider && TouchedActor == Hider)
    {
        FinishRound(true);
    }
}

void AHumanityTrinityRebuildHideAndSeekGameMode::FinishRound(const bool bCaught)
{
    bRoundRunning = false;
    bRoundFinished = true;
    bSeekerWon = bCaught;
    for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It)
    {
        It->SetMasterLights(true);
        break;
    }
    if (Hider)
    {
        Hider->SetSeeker(nullptr);
    }
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] ROUND_FINISHED winner=%s remaining=%.1f"),
           bCaught ? TEXT("SEEKER") : TEXT("HIDER"), SecondsRemaining);
}

void AHumanityTrinityRebuildHideAndSeekGameMode::RestartRound()
{
    GetWorldTimerManager().ClearTimer(PreparationTimer);
    BeginPreparation();
}

FString AHumanityTrinityRebuildHideAndSeekGameMode::GetStatusLine() const
{
    if (bRoundFinished)
    {
        return bSeekerWon ? TEXT("FOUND THEM!  Press R for another round")
                          : TEXT("TIME UP - hider wins.  Press R to retry");
    }
    if (!bRoundRunning)
    {
        const int32 Count = FMath::Max(0, FMath::CeilToInt(PreparationEndsAt - GetWorld()->GetTimeSeconds()));
        return bPracticeMode
                   ? FString::Printf(TEXT("PRACTICE  |  Round starts in %d  |  Tab: switch to darkness"), Count)
                   : FString::Printf(TEXT("MEMORIZE THE ROOM  |  Lights out in %d  |  Tab: bright practice"), Count);
    }
    return FString::Printf(TEXT("%s  |  %d seconds left"),
                           bPracticeMode ? TEXT("BRIGHT PRACTICE") : TEXT("FIND THE HIDER"),
                           FMath::CeilToInt(SecondsRemaining));
}

void AHumanityTrinityRebuildHideAndSeekGameMode::TogglePracticeMode()
{
    if (bSelfTest)
        return;
    bPracticeMode = !bPracticeMode;
    RestartRound();
}
void AHumanityTrinityRebuildHideAndSeekGameMode::ReportSeekerNoise(const FVector& Location, float Loudness)
{
    if (bRoundRunning && Hider)
        Hider->HearNoise(Location, Loudness);
}
FString AHumanityTrinityRebuildHideAndSeekGameMode::GetPracticeHint() const
{
    return bPracticeMode && bRoundRunning && Hider ? Hider->GetBehaviorLabel() : FString();
}

void AHumanityTrinityRebuildHideAndSeekGameMode::RunSelfTest()
{
    bSelfTestCorePassed = Seeker && Hider && Hider->HasFootstepAudio() && Hider->GetLoadedPartCount() == 15 &&
                          Hider->GetCoverCount() >= 4 && SecondsRemaining < 179.9f;
    for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It)
    {
        bSelfTestCorePassed &= It->GetActiveMainLightCount() == 0 && It->GetActiveResidualLightCount() == 0;
        break;
    }
    if (!Hider || !Seeker)
    {
        FinishSelfTest();
        return;
    }
    SelfTestHiderStart = Hider->GetActorLocation();
    // Physical proximity alone must not reveal the seeker to the hider.
    Seeker->SetActorLocation(SelfTestHiderStart + FVector(0, 100, 26));
    Seeker->Crouch();
    FTimerHandle QuietTimer;
    GetWorldTimerManager().SetTimer(
        QuietTimer,
        [this]() {
            const bool Quiet = Hider->GetHeardCount() == 0 && Hider->GetStepsEmitted() == 0 &&
                               FVector::Dist2D(SelfTestHiderStart, Hider->GetActorLocation()) < 1;
            const bool Crouch = Seeker->bIsCrouched && Seeker->GetFootstepLoudness() < .1f && Hider->IsCrouching();
            Hider->HearNoise(Hider->GetActorLocation() + FVector(220, 0, 0), .09f);
            const bool QuietRange = Hider->GetHeardCount() == 0;
            bSelfTestCorePassed &= Quiet && Crouch && QuietRange;
            UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK_SELFTEST] proximity_silent=%s crouch=%s quiet_range=%s"),
                   Quiet ? TEXT("PASS") : TEXT("FAIL"), Crouch ? TEXT("PASS") : TEXT("FAIL"),
                   QuietRange ? TEXT("PASS") : TEXT("FAIL"));
            Seeker->UnCrouch();
            Seeker->SetActorLocation(FVector(0, -220, 96));
            ReportSeekerNoise(Hider->GetActorLocation() + FVector(0, 180, 0), .85f);
            bSelfTestCorePassed &= Hider->GetHeardCount() == 1;
            FTimerHandle FinishTimer;
            GetWorldTimerManager().SetTimer(FinishTimer, this,
                                            &AHumanityTrinityRebuildHideAndSeekGameMode::FinishSelfTest, 22.f, false);
        },
        1.5f, false);
}

void AHumanityTrinityRebuildHideAndSeekGameMode::FinishSelfTest()
{
    bool bPass = bSelfTestCorePassed;
    if (Seeker && Hider)
    {
        const float Distance = FVector::Dist2D(Hider->GetActorLocation(), SelfTestHiderStart);
        bPass &= Distance > 220 && Hider->GetStepsEmitted() >= 5 && Hider->GetRouteLength() > 3;
        UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK_SELFTEST] movement_cm=%.1f footsteps=%d route=%d settled=%s"),
               Distance, Hider->GetStepsEmitted(), Hider->GetRouteLength(),
               Hider->GetHideState() == EHiderState::Hidden ? TEXT("YES") : TEXT("NO"));
        // Use a clear area to validate a real articulated touch, including occlusion.
        Hider->SetShowcasePose(0);
        Hider->SetActorLocation(FVector(0, -300, 90));
        Hider->SetActorRotation(FRotator::ZeroRotator);
        Seeker->SetActorLocation(FVector(95, -300, 96));
        Seeker->GetController()->SetControlRotation(FRotator(-10, 180, 0));
        AActor* Barrier = GetWorld()->SpawnActor<AActor>();
        UBoxComponent* Box = NewObject<UBoxComponent>(Barrier);
        Barrier->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(4, 60, 90));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionResponseToAllChannels(ECR_Ignore);
        Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        Box->RegisterComponent();
        Barrier->SetActorLocation(FVector(48, -300, 90));
        Seeker->PerformTouch();
        const bool Blocked = !bRoundFinished && Seeker->GetCurrentTouchMessage().Contains(TEXT("A firm surface"));
        Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Barrier->Destroy();
        Seeker->PerformTouch();
        bPass &= Blocked && bRoundFinished && bSeekerWon;
        UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK_SELFTEST] touch_occlusion=%s catch=%s"),
               Blocked ? TEXT("PASS") : TEXT("FAIL"), bSeekerWon ? TEXT("PASS") : TEXT("FAIL"));
    }
    bSelfTestCorePassed = bPass;
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK_SELFTEST] core=%s"), bPass ? TEXT("PASS") : TEXT("FAIL"));
    FTimerHandle CaptureTimer;
    GetWorldTimerManager().SetTimer(CaptureTimer, this, &AHumanityTrinityRebuildHideAndSeekGameMode::CaptureFound, .7f,
                                    false);
    FTimerHandle ShowcaseTimer;
    GetWorldTimerManager().SetTimer(ShowcaseTimer, this,
                                    &AHumanityTrinityRebuildHideAndSeekGameMode::CaptureCharacterShowcase, 1.2f, false);
}

void AHumanityTrinityRebuildHideAndSeekGameMode::CaptureCharacterShowcase()
{
    if (!Hider || !Seeker)
    {
        ExitSelfTest();
        return;
    }
    if (ShowcaseStep == 0)
    {
        Seeker->SetActorLocation(FVector(255, -125, 96));
        Seeker->GetController()->SetControlRotation((FVector(0, -300, 88) - FVector(255, -125, 162)).Rotation());
    }
    if (ShowcaseStep > 0)
        CaptureGameScreenshot(FString::Printf(
            TEXT("0%d_Character_%s.png"), ShowcaseStep + 3,
            ShowcaseStep == 1 ? TEXT("Standing") : (ShowcaseStep == 2 ? TEXT("Crouching") : TEXT("Walking"))));
    if (ShowcaseStep >= 3)
    {
        FTimerHandle End;
        GetWorldTimerManager().SetTimer(End, this, &AHumanityTrinityRebuildHideAndSeekGameMode::TestRoundControls, .7f,
                                        false);
        return;
    }
    Hider->SetShowcasePose(ShowcaseStep);
    ++ShowcaseStep;
    FTimerHandle Next;
    GetWorldTimerManager().SetTimer(Next, this, &AHumanityTrinityRebuildHideAndSeekGameMode::CaptureCharacterShowcase,
                                    1.5f, false);
}

void AHumanityTrinityRebuildHideAndSeekGameMode::TestRoundControls()
{
    // Exercise production round controls without scheduling the self-test recursively.
    bSelfTest = false;
    TogglePracticeMode();
    const bool Restarted = bPracticeMode && !bRoundRunning && !bRoundFinished && !Hider;
    GetWorldTimerManager().ClearTimer(PreparationTimer);
    StartRound();
    bool Lit = false;
    for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It)
    {
        Lit = It->GetActiveMainLightCount() > 0;
        break;
    }
    bSelfTestCorePassed &= Restarted && Lit && bRoundRunning && Hider;
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK_SELFTEST] practice_restart=%s practice_lit=%s"),
           Restarted ? TEXT("PASS") : TEXT("FAIL"), Lit ? TEXT("PASS") : TEXT("FAIL"));
    if (!Hider)
    {
        ExitSelfTest();
        return;
    }
    Hider->SetActorLocation(FVector(150, -220, 90));
    SelfTestMoveUntil = GetWorld()->GetTimeSeconds() + .45f;
    FTimerHandle Movement;
    GetWorldTimerManager().SetTimer(
        Movement,
        [this]() {
            SelfTestMoveUntil = 0;
            Seeker->GetCharacterMovement()->StopMovementImmediately();
            const bool StepsHeard = Hider->GetHeardCount() > 0;
            bSelfTestCorePassed &= StepsHeard;
            UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK_SELFTEST] actual_player_footsteps=%s heard=%d"),
                   StepsHeard ? TEXT("PASS") : TEXT("FAIL"), Hider->GetHeardCount());
            CaptureGameScreenshot(TEXT("07_BrightPractice.png"));
            SecondsRemaining = .05f;
            FTimerHandle Timeout;
            GetWorldTimerManager().SetTimer(
                Timeout,
                [this]() {
                    const bool TimedOut = bRoundFinished && !bRoundRunning && !bSeekerWon;
                    TogglePracticeMode();
                    const bool BackToDark = !bPracticeMode && !bRoundRunning && !bRoundFinished && !Hider;
                    bSelfTestCorePassed &= TimedOut && BackToDark;
                    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK_SELFTEST] timeout=%s dark_restart=%s"),
                           TimedOut ? TEXT("PASS") : TEXT("FAIL"), BackToDark ? TEXT("PASS") : TEXT("FAIL"));
                    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK_SELFTEST] %s"),
                           bSelfTestCorePassed ? TEXT("PASS") : TEXT("FAIL"));
                    ExitSelfTest();
                },
                .3f, false);
        },
        .6f, false);
}

void AHumanityTrinityRebuildHideAndSeekGameMode::CaptureMemorize()
{
    CaptureGameScreenshot(TEXT("01_Memorize.png"));
}

void AHumanityTrinityRebuildHideAndSeekGameMode::CaptureBlackout()
{
    CaptureGameScreenshot(TEXT("02_Blackout.png"));
}

void AHumanityTrinityRebuildHideAndSeekGameMode::CaptureFound()
{
    CaptureGameScreenshot(TEXT("03_Found.png"));
}

void AHumanityTrinityRebuildHideAndSeekGameMode::ExitSelfTest()
{
    FPlatformMisc::RequestExit(false);
}

void AHumanityTrinityRebuildHideAndSeekGameMode::CaptureGameScreenshot(const FString& FileName)
{
    if (!bCaptureGame)
        return;
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), TEXT("HideAndSeek"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString FullPath = FPaths::Combine(Directory, FileName);
    FScreenshotRequest::RequestScreenshot(FullPath, false, false);
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] SCREENSHOT_REQUESTED %s"), *FullPath);
}
