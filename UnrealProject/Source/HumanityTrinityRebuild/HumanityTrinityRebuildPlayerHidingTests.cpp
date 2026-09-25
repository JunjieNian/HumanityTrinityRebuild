#include "HumanityTrinityRebuildHideAndSeekGameMode.h"

#include "HumanityTrinityRebuildSeeker.h"
#include "HumanityTrinityRebuildPlayerCharacter.h"
#include "HumanityTrinityRebuildLightingController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void AHumanityTrinityRebuildHideAndSeekGameMode::RecordPlayerHidingCheck(const TCHAR* Name, bool bPassed)
{
    bPlayerHidingTestPassed &= bPassed;
    UE_LOG(LogTemp, Display, TEXT("[PLAYER_HIDING_SELFTEST] %s=%s"), Name, bPassed ? TEXT("PASS") : TEXT("FAIL"));
}

void AHumanityTrinityRebuildHideAndSeekGameMode::RunPlayerHidingSelfTest()
{
    bPlayerHidingSelfTest = false; // Later production restarts must not schedule another test.
    RecordPlayerHidingCheck(TEXT("role_and_preparation"), bPlayerHiding && !Hider && SearchingNPC && Seeker &&
        !bRoundRunning && !bRoundFinished && PreparationEndsAt - GetWorld()->GetTimeSeconds() > 18);
    if (!SearchingNPC || !Seeker)
    {
        UE_LOG(LogTemp, Error, TEXT("[PLAYER_HIDING_SELFTEST] FAIL missing actors"));
        ExitSelfTest();
        return;
    }
    RecordPlayerHidingCheck(TEXT("model_and_audio"), SearchingNPC->GetLoadedPartCount() == 15 &&
        SearchingNPC->HasFootstepAudio());
    SearchingNPC->HearPlayerNoise(SearchingNPC->GetActorLocation(), 1.f);
    RecordPlayerHidingCheck(TEXT("counting_does_not_hear"), SearchingNPC->GetHeardCount() == 0 &&
        SearchingNPC->GetSearchState() == ETrinitySearchState::Waiting);
    FHitResult Hit;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(HidingBoundaryTest), true, Seeker);
    const bool bRail = GetWorld()->LineTraceSingleByChannel(Hit, FVector(0, -1320, 150),
        FVector(0, -1430, 150), ECC_Visibility, Q) && Hit.GetActor() == HidingBoundary;
    RecordPlayerHidingCheck(TEXT("stage_boundary"), bRail);
    Seeker->SetActorLocation(FVector(-160, -130, 96));
    Seeker->GetController()->SetControlRotation((FVector(150, -220, 100) - FVector(-160, -130, 162)).Rotation());
    CaptureGameScreenshot(TEXT("08_PlayerHiding_Preparation.png"));
    FTimerHandle Next;
    GetWorldTimerManager().SetTimer(Next, this,
        &AHumanityTrinityRebuildHideAndSeekGameMode::TestPlayerHidingSound, 1.f, false);
}

void AHumanityTrinityRebuildHideAndSeekGameMode::TestPlayerHidingSound()
{
    ReadyToHide();
    RecordPlayerHidingCheck(TEXT("navigation_uses_loaded_furniture"), SearchingNPC->Nodes.Num() > 240 &&
        SearchingNPC->Nodes.Num() < 600 && SearchingNPC->GetCoverCount() >= 5);
    RecordPlayerHidingCheck(TEXT("ready_starts_search"), bRoundRunning && bPlayerHiding &&
        !GetWorldTimerManager().IsTimerActive(PreparationTimer));
    bool bDark = false;
    for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It)
        bDark = It->GetActiveMainLightCount() == 0 && It->GetActiveResidualLightCount() == 0;
    RecordPlayerHidingCheck(TEXT("dark_round"), bDark && !bPracticeMode);

    // Keep the listener test's player away from the patrol so an incidental,
    // legitimate touch does not end the round before the contact checks.
    auto PlaceAwayFromRoute = [this]() {
        float Best = -1;
        FVector Position = FVector::ZeroVector;
        for (const auto& Node : SearchingNPC->Nodes)
        {
            float Clearance = FLT_MAX;
            for (int32 Step : SearchingNPC->Route)
                Clearance = FMath::Min(Clearance, float(FVector::Dist2D(Node.Position, SearchingNPC->Nodes[Step].Position)));
            if (Clearance > Best) { Best = Clearance; Position = Node.Position; }
        }
        Seeker->SetActorLocation(Position + FVector(0, 0, 6));
    };
    PlaceAwayFromRoute();
    Seeker->Crouch();
    const TArray<int32> OriginalPatrol = SearchingNPC->Route;
    FTimerHandle Quiet;
    GetWorldTimerManager().SetTimer(Quiet, [this, OriginalPatrol, PlaceAwayFromRoute]() {
        RecordPlayerHidingCheck(TEXT("silent_player_not_tracked"), SearchingNPC->GetHeardCount() == 0 &&
            !SearchingNPC->bHasNoiseMemory && SearchingNPC->Route == OriginalPatrol && !bRoundFinished);
        RecordPlayerHidingCheck(TEXT("player_crouch"), Seeker->bIsCrouched && Seeker->GetFootstepLoudness() < .1f);
        CaptureGameScreenshot(TEXT("09_PlayerHiding_Dark.png"));

        SearchingNPC->SetActorLocation(FVector(0, -300, 90));
        SearchingNPC->SetActorRotation(FRotator::ZeroRotator);
        SearchingNPC->HearPlayerNoise(FVector(220, -300, 90), .09f);
        RecordPlayerHidingCheck(TEXT("quiet_steps_have_short_range"), SearchingNPC->GetHeardCount() == 0);

        AActor* Barrier = GetWorld()->SpawnActor<AActor>();
        UBoxComponent* Box = NewObject<UBoxComponent>(Barrier);
        Barrier->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(5, 90, 140));
        Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Box->SetCollisionResponseToAllChannels(ECR_Ignore);
        Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        Box->RegisterComponent();
        Barrier->SetActorLocation(FVector(90, -300, 100));
        SearchingNPC->HearPlayerNoise(FVector(180, -300, 90), .35f);
        RecordPlayerHidingCheck(TEXT("sound_obstruction"), SearchingNPC->GetHeardCount() == 0);
        Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Barrier->Destroy();

        ReportPlayerNoise(FVector(0, -560, 90), .85f);
        RecordPlayerHidingCheck(TEXT("audible_event_causes_listening"), SearchingNPC->GetHeardCount() == 1 &&
            SearchingNPC->GetSearchState() == ETrinitySearchState::Listening);
        const FVector Remembered = SearchingNPC->LastHeard;
        FTimerHandle Memory;
        GetWorldTimerManager().SetTimer(Memory, [this, Remembered, PlaceAwayFromRoute]() {
            PlaceAwayFromRoute();
            RecordPlayerHidingCheck(TEXT("investigates_snapshot_not_player"),
                SearchingNPC->LastHeard.Equals(Remembered, .01f) && SearchingNPC->GetHeardCount() == 1 &&
                SearchingNPC->GetSearchState() == ETrinitySearchState::Investigating &&
                SearchingNPC->GetRouteLength() > 1);
            // Furniture can make a nearby sound require a long walk around a row.
            const float ArrivalTime = FMath::Clamp(SearchingNPC->GetRouteLength() * 40.f / 110.f + 1.f, 4.f, 45.f);
            FTimerHandle Move;
            GetWorldTimerManager().SetTimer(Move, this,
                &AHumanityTrinityRebuildHideAndSeekGameMode::TestPlayerHidingContact, ArrivalTime, false);
        }, 1.3f, false);
    }, 1.4f, false);
}

void AHumanityTrinityRebuildHideAndSeekGameMode::TestPlayerHidingContact()
{
    UE_LOG(LogTemp, Display, TEXT("[PLAYER_HIDING_SELFTEST] observed_travel=%.1f steps=%d position=%s sound=%s"),
        SearchingNPC->DistanceTravelled, SearchingNPC->GetStepsEmitted(),
        *SearchingNPC->GetActorLocation().ToString(), *SearchingNPC->LastHeard.ToString());
    RecordPlayerHidingCheck(TEXT("npc_moves_and_emits_footsteps"), SearchingNPC->DistanceTravelled > 180 &&
        SearchingNPC->GetStepsEmitted() >= 4);
    RecordPlayerHidingCheck(TEXT("searches_after_arrival"), SearchingNPC->StopsSearched > 0);
    RecordPlayerHidingCheck(TEXT("reaches_sound_area"),
        FVector::Dist2D(SearchingNPC->GetActorLocation(), SearchingNPC->LastHeard) < 220);
    SearchingNPC->SetActorTickEnabled(false);
    SearchingNPC->SetActorLocation(FVector(0, -300, 90));
    SearchingNPC->SetActorRotation(FRotator::ZeroRotator);
    SearchingNPC->Body->SetCapsuleHalfHeight(88);
    Seeker->GetCharacterMovement()->StopMovementImmediately();
    Seeker->SetActorLocation(FVector(95, -300, 62));
    AActor* Barrier = GetWorld()->SpawnActor<AActor>();
    UBoxComponent* Box = NewObject<UBoxComponent>(Barrier);
    Barrier->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(4, 80, 120));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore);
    Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Box->RegisterComponent();
    Barrier->SetActorLocation(FVector(48, -300, 90));
    const bool bBlocked = !SearchingNPC->FeelAhead() && !bRoundFinished;
    RecordPlayerHidingCheck(TEXT("cannot_catch_through_wall"), bBlocked);
    Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Barrier->Destroy();
    const bool bCaught = SearchingNPC->FeelAhead();
    RecordPlayerHidingCheck(TEXT("physical_crouched_contact_loses"), bCaught && bRoundFinished && bSeekerWon &&
        !bRoundRunning && SearchingNPC->GetSearchState() == ETrinitySearchState::Finished);
    Seeker->UnCrouch();
    Seeker->SetActorLocation(FVector(240, -140, 96));
    Seeker->GetController()->SetControlRotation((FVector(0, -300, 98) - FVector(240, -140, 162)).Rotation());
    FTimerHandle Controls;
    GetWorldTimerManager().SetTimer(Controls, this,
        &AHumanityTrinityRebuildHideAndSeekGameMode::TestPlayerHidingRoundControls, .7f, false);
}

void AHumanityTrinityRebuildHideAndSeekGameMode::TestPlayerHidingRoundControls()
{
    CaptureGameScreenshot(TEXT("10_PlayerHiding_Found.png"));
    FTimerHandle Practice;
    GetWorldTimerManager().SetTimer(Practice, [this]() {
        TogglePracticeMode();
        RecordPlayerHidingCheck(TEXT("practice_restart_keeps_role"), bPlayerHiding && bPracticeMode &&
            !bRoundRunning && !bRoundFinished && SearchingNPC && !Hider && SearchingNPC->GetHeardCount() == 0);
        ReadyToHide();
        bool bLit = false;
        for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It)
            bLit = It->GetActiveMainLightCount() > 0;
        RecordPlayerHidingCheck(TEXT("practice_round_is_lit"), bLit && bRoundRunning);
        SearchingNPC->SetActorTickEnabled(false);
        SelfTestMoveUntil = GetWorld()->GetTimeSeconds() + .45f;
        FTimerHandle Steps;
        GetWorldTimerManager().SetTimer(Steps, [this]() {
            SelfTestMoveUntil = 0;
            Seeker->GetCharacterMovement()->StopMovementImmediately();
            RecordPlayerHidingCheck(TEXT("actual_player_steps_are_heard"), SearchingNPC->GetHeardCount() > 0);
            Seeker->SetActorLocation(FVector(-160, -130, 96));
            Seeker->GetController()->SetControlRotation((SearchingNPC->GetActorLocation() + FVector(0, 0, 15)
                - FVector(-160, -130, 162)).Rotation());
            CaptureGameScreenshot(TEXT("11_PlayerHiding_Practice.png"));
            FTimerHandle Timeout;
            GetWorldTimerManager().SetTimer(Timeout, [this]() {
                SecondsRemaining = .05f;
                FTimerHandle Result;
                GetWorldTimerManager().SetTimer(Result, [this]() {
                    RecordPlayerHidingCheck(TEXT("surviving_timer_wins"), bRoundFinished && !bSeekerWon && !bRoundRunning &&
                        GetStatusLine().Contains(TEXT("YOU STAYED HIDDEN")));
                    CaptureGameScreenshot(TEXT("12_PlayerHiding_Win.png"));
                    FTimerHandle Restart;
                    GetWorldTimerManager().SetTimer(Restart, [this]() {
                        TogglePracticeMode();
                        RecordPlayerHidingCheck(TEXT("dark_restart_resets_round"), bPlayerHiding && !bPracticeMode &&
                            !bRoundRunning && !bRoundFinished && SearchingNPC && SearchingNPC->GetHeardCount() == 0);
                        int32 NPCs = 0;
                        for (TActorIterator<AHumanityTrinityRebuildSeeker> It(GetWorld()); It; ++It)
                            if (!It->IsActorBeingDestroyed()) ++NPCs;
                        RecordPlayerHidingCheck(TEXT("restart_has_one_npc"), NPCs == 1);
                        UE_LOG(LogTemp, Display, TEXT("[PLAYER_HIDING_SELFTEST] %s"),
                            bPlayerHidingTestPassed ? TEXT("PASS") : TEXT("FAIL"));
                        ExitSelfTest();
                    }, .5f, false);
                }, .3f, false);
            }, .6f, false);
        }, .65f, false);
    }, .6f, false);
}
