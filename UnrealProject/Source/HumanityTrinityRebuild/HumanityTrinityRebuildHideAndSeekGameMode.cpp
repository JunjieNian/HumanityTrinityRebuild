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

void AHumanityTrinityRebuildHideAndSeekGameMode::BeginPlay()
{
    Super::BeginPlay();
    bSelfTest = FParse::Param(FCommandLine::Get(), TEXT("HideAndSeekSelfTest"));
    bCaptureGame = FParse::Param(FCommandLine::Get(), TEXT("HideAndSeekCapture"));
    GetWorldTimerManager().SetTimerForNextTick(this,
        &AHumanityTrinityRebuildHideAndSeekGameMode::BeginPreparation);
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
        GetWorldTimerManager().SetTimer(PreparationTimer, this,
            &AHumanityTrinityRebuildHideAndSeekGameMode::StartRound, 1.0f, false);
    }
    else
    {
        GetWorldTimerManager().SetTimer(PreparationTimer, this,
            &AHumanityTrinityRebuildHideAndSeekGameMode::StartRound, 12.0f, false);
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
        It->SetMasterLights(false);
        break;
    }
    const float Side = bSelfTest ? 1.0f : (FMath::RandBool() ? 1.0f : -1.0f);
    const FVector HideLocation(Side * 165.0f, -900.0f, 91.0f);
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Hider = GetWorld()->SpawnActor<AHumanityTrinityRebuildHider>(
        AHumanityTrinityRebuildHider::StaticClass(), HideLocation, FRotator::ZeroRotator, Spawn);
    if (!Hider)
    {
        UE_LOG(LogTemp, Error, TEXT("[HIDE_AND_SEEK] SETUP_FAIL hider spawn"));
        return;
    }
    Hider->SetSeeker(Seeker);
    bRoundRunning = true;
    SecondsRemaining = 180.0f;
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] ROUND_STARTED residual_lights=OFF hider=%s"),
        *HideLocation.ToString());
    if (bSelfTest)
    {
        FTimerHandle CaptureTimer;
        GetWorldTimerManager().SetTimer(CaptureTimer, this,
            &AHumanityTrinityRebuildHideAndSeekGameMode::CaptureBlackout, 0.30f, false);
        FTimerHandle TestTimer;
        GetWorldTimerManager().SetTimer(TestTimer, this,
            &AHumanityTrinityRebuildHideAndSeekGameMode::RunSelfTest, 0.65f, false);
    }
}

void AHumanityTrinityRebuildHideAndSeekGameMode::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
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
        return FString::Printf(TEXT("MEMORIZE THE ROOM  |  Lights out in %d"), Count);
    }
    return FString::Printf(TEXT("FIND THE HIDER  |  %d seconds left"),
        FMath::CeilToInt(SecondsRemaining));
}

void AHumanityTrinityRebuildHideAndSeekGameMode::RunSelfTest()
{
    bSelfTestCorePassed = Seeker && Hider && Hider->HasFootstepAudio();
    for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It)
    {
        bSelfTestCorePassed &= It->GetActiveMainLightCount() == 0;
        bSelfTestCorePassed &= It->GetActiveResidualLightCount() == 0;
        break;
    }
    if (Seeker && Hider)
    {
        FCollisionQueryParams RouteQuery(SCENE_QUERY_STAT(HideAndSeekAccessRoute), false);
        RouteQuery.AddIgnoredActor(Seeker);
        RouteQuery.AddIgnoredActor(Hider);
        for (const float RouteX : {165.0f, 470.0f, 500.0f, -470.0f, -500.0f})
        {
            const FVector First(0.0f, -220.0f, 96.0f);
            const FVector Bend(RouteX, -220.0f, 96.0f);
            const FVector Last(RouteX, -900.0f, 96.0f);
            FHitResult Hit;
            const FCollisionShape Shape = FCollisionShape::MakeCapsule(34.0f, 92.0f);
            const bool bAcrossBlocked = GetWorld()->SweepSingleByChannel(Hit, First, Bend,
                FQuat::Identity, ECC_Pawn, Shape, RouteQuery);
            const bool bAlongBlocked = !bAcrossBlocked && GetWorld()->SweepSingleByChannel(Hit, Bend, Last,
                FQuat::Identity, ECC_Pawn, Shape, RouteQuery);
            UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK_SELFTEST] route_x=%.0f across=%s along=%s hit=%s at=%s"),
                RouteX, bAcrossBlocked ? TEXT("BLOCKED") : TEXT("CLEAR"),
                bAlongBlocked ? TEXT("BLOCKED") : TEXT("CLEAR"),
                Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("none"),
                *Hit.Location.ToString());
        }
        for (const float TargetX : {165.0f, -165.0f})
        for (const float Radius : {34.0f, 24.0f})
        {
            constexpr int32 CellsX = 27;
            constexpr int32 CellsY = 29;
            constexpr float CellSize = 40.0f;
            auto GridPosition = [](const int32 Index)
            {
                return FVector(-520.0f + (Index % CellsX) * CellSize,
                    -220.0f - (Index / CellsX) * CellSize, 96.0f);
            };
            TArray<int32> Parent;
            Parent.Init(-2, CellsX * CellsY);
            TArray<int32> Queue;
            const int32 Start = 13;
            Parent[Start] = -1;
            Queue.Add(Start);
            int32 Found = -1;
            for (int32 Cursor = 0; Cursor < Queue.Num() && Found < 0; ++Cursor)
            {
                const int32 Current = Queue[Cursor];
                const FVector From = GridPosition(Current);
                if (FVector::Dist2D(From, FVector(TargetX, -900.0f, 96.0f)) < 100.0f)
                {
                    Found = Current;
                    break;
                }
                const int32 Column = Current % CellsX;
                const int32 Row = Current / CellsX;
                for (const FIntPoint Offset : {FIntPoint(1,0), FIntPoint(-1,0),
                    FIntPoint(0,1), FIntPoint(0,-1)})
                {
                    const int32 X = Column + Offset.X;
                    const int32 Y = Row + Offset.Y;
                    if (X < 0 || X >= CellsX || Y < 0 || Y >= CellsY) continue;
                    const int32 Next = Y * CellsX + X;
                    if (Parent[Next] != -2) continue;
                    FHitResult RouteHit;
                    const bool bBlocked = GetWorld()->SweepSingleByChannel(RouteHit,
                        From, GridPosition(Next), FQuat::Identity, ECC_Pawn,
                        FCollisionShape::MakeCapsule(Radius, 92.0f), RouteQuery);
                    if (bBlocked) continue;
                    Parent[Next] = Current;
                    Queue.Add(Next);
                }
            }
            if (FMath::IsNearlyEqual(Radius, 24.0f))
            {
                bSelfTestCorePassed &= Found >= 0;
            }
            UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK_SELFTEST] grid_target_x=%.0f radius=%.0f reachable=%s explored=%d end=%s"),
                TargetX, Radius, Found >= 0 ? TEXT("YES") : TEXT("NO"), Queue.Num(),
                Found >= 0 ? *GridPosition(Found).ToString() : TEXT("none"));
        }
        SelfTestHiderStart = Hider->GetActorLocation();
        Seeker->SetActorLocation(SelfTestHiderStart + FVector(0.0f, 230.0f, 0.0f),
            false, nullptr, ETeleportType::TeleportPhysics);
        FTimerHandle FinishTimer;
        GetWorldTimerManager().SetTimer(FinishTimer, this,
            &AHumanityTrinityRebuildHideAndSeekGameMode::FinishSelfTest, 3.0f, false);
    }
    else
    {
        FinishSelfTest();
    }
}

void AHumanityTrinityRebuildHideAndSeekGameMode::FinishSelfTest()
{
    bool bPass = bSelfTestCorePassed;
    if (Seeker && Hider)
    {
        const float DistanceMoved = FVector::Dist2D(Hider->GetActorLocation(), SelfTestHiderStart);
        const int32 Footsteps = Hider->GetStepsEmitted();
        bPass &= DistanceMoved > 90.0f && Footsteps >= 2;
        UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK_SELFTEST] movement_cm=%.1f footsteps=%d"),
            DistanceMoved, Footsteps);
        // Exercise the player's real near-hand touch and the win transition.
        Seeker->SetActorLocation(Hider->GetActorLocation() + FVector(0.0f, 94.0f, 0.0f),
            false, nullptr, ETeleportType::TeleportPhysics);
        Seeker->GetController()->SetControlRotation(FRotator(0.0f, -90.0f, 0.0f));
        Seeker->PerformTouch();
        bPass &= bRoundFinished && bSeekerWon;
    }
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK_SELFTEST] %s audio=%s catch=%s"),
        bPass ? TEXT("PASS") : TEXT("FAIL"),
        Hider && Hider->HasFootstepAudio() ? TEXT("YES") : TEXT("NO"),
        bSeekerWon ? TEXT("YES") : TEXT("NO"));
    FTimerHandle CaptureTimer;
    GetWorldTimerManager().SetTimer(CaptureTimer, this,
        &AHumanityTrinityRebuildHideAndSeekGameMode::CaptureFound, 0.55f, false);
    FTimerHandle ExitTimer;
    GetWorldTimerManager().SetTimer(ExitTimer, this,
        &AHumanityTrinityRebuildHideAndSeekGameMode::ExitSelfTest, 1.0f, false);
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
    if (!bCaptureGame) return;
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), TEXT("HideAndSeek"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString FullPath = FPaths::Combine(Directory, FileName);
    FScreenshotRequest::RequestScreenshot(FullPath, false, false);
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] SCREENSHOT_REQUESTED %s"), *FullPath);
}
