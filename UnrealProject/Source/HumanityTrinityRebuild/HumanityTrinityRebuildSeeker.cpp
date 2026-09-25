#include "HumanityTrinityRebuildSeeker.h"

#include "HumanityTrinityRebuildHideAndSeekGameMode.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"

void AHumanityTrinityRebuildSeeker::PrepareToSeek(AActor* Player)
{
    Seeker = Player; // Shared geometry queries ignore the other participant.
    bSearching = false;
    bHasNoiseMemory = false;
    HeardCount = 0;
    SetSearchState(ETrinitySearchState::Waiting);
    UE_LOG(LogTemp, Display, TEXT("[NPC_SEEKER] WAITING parts=%d"), GetLoadedPartCount());
}

void AHumanityTrinityRebuildSeeker::StartSearching()
{
    // Static mesh collision can still be completing during map BeginPlay. The
    // lit preparation allows it to settle before we sample the walkable floor.
    BuildRoutes(true);
    LastVisited.Init(-1000.f, Nodes.Num());
    bSearching = true;
    ChoosePatrolStop();
}

void AHumanityTrinityRebuildSeeker::StopSearching()
{
    bSearching = false;
    Route.Empty();
    SetSearchState(ETrinitySearchState::Finished);
}

void AHumanityTrinityRebuildSeeker::HearPlayerNoise(const FVector& Location, float Loudness)
{
    if (!bSearching || !Seeker)
        return;
    float Range = FMath::Clamp(Loudness, 0.f, 1.f) * 700.f;
    const float Distance = FVector::Dist2D(Location, GetActorLocation());
    FCollisionQueryParams Q(SCENE_QUERY_STAT(SeekerHearing), true, this);
    Q.AddIgnoredActor(Seeker);
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Location + FVector(0, 0, 30),
            GetActorLocation() + FVector(0, 0, 45), ECC_Visibility, Q))
        Range *= .5f;
    if (Range <= 0 || Distance > Range)
        return;

    // The sound supplies a noisy snapshot. Subsequent silent movement does not update it.
    const float Error = FMath::Clamp(Distance * .12f, 12.f, 65.f);
    const float Angle = FMath::FRandRange(-PI, PI);
    LastHeard = Location + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Error;
    const float Now = GetWorld()->GetTimeSeconds();
    MemoryUntil = Now + 9.f;
    bHasNoiseMemory = true;
    ++HeardCount;
    if (SearchState != ETrinitySearchState::Listening &&
        SearchState != ETrinitySearchState::Investigating && Now >= NextReactionAt)
    {
        Route.Empty();
        SetSearchState(ETrinitySearchState::Listening);
        PauseUntil = Now + .65f;
        NextReactionAt = Now + 1.6f;
    }
    UE_LOG(LogTemp, Display, TEXT("[NPC_SEEKER] HEARD distance=%.0f loudness=%.2f remembered=%s"),
           Distance, Loudness, *LastHeard.ToString());
}

bool AHumanityTrinityRebuildSeeker::PlanTo(int32 Goal)
{
    const int32 Start = NearestNode(GetActorLocation());
    if (Start == INDEX_NONE || !Nodes.IsValidIndex(Goal))
        return false;
    TArray<int32> Parent, Queue;
    Parent.Init(INDEX_NONE, Nodes.Num());
    Parent[Start] = Start;
    Queue.Add(Start);
    for (int32 Cursor = 0; Cursor < Queue.Num() && Parent[Goal] == INDEX_NONE; ++Cursor)
    {
        const int32 Current = Queue[Cursor];
        for (int32 Next : Nodes[Current].Neighbors)
        {
            if (Parent[Next] != INDEX_NONE ||
                (Next == AvoidNode && GetWorld()->GetTimeSeconds() < AvoidUntil))
                continue;
            if (!IsPassageClear(Nodes[Current].Position, Nodes[Next].Position))
                continue;
            Parent[Next] = Current;
            Queue.Add(Next);
        }
    }
    if (Parent[Goal] == INDEX_NONE)
        return false;
    Route.Empty();
    for (int32 N = Goal; ; N = Parent[N])
    {
        Route.Insert(N, 0);
        if (N == Start)
            break;
    }
    RouteCursor = 0;
    BlockedSeconds = 0;
    UE_LOG(LogTemp, Display, TEXT("[NPC_SEEKER] ROUTE nodes=%d goal=%s"), Route.Num(), *Nodes[Goal].Position.ToString());
    return true;
}

void AHumanityTrinityRebuildSeeker::ChoosePatrolStop()
{
    bHasNoiseMemory = false;
    LocalSearchesLeft = 0;
    const float Now = GetWorld()->GetTimeSeconds();
    TArray<TPair<float, int32>> Candidates;
    for (int32 I = 0; I < Nodes.Num(); ++I)
    {
        const float Distance = FVector::Dist2D(GetActorLocation(), Nodes[I].Position);
        if (Distance < 150 || (I == AvoidNode && Now < AvoidUntil))
            continue;
        // Search unvisited furniture and floor sectors without consulting the player.
        const float Score = FMath::Min(Now - LastVisited[I], 400.f) + Nodes[I].Cover * 28.f
                            - Distance * .07f + FMath::FRandRange(0.f, 30.f);
        Candidates.Emplace(Score, I);
    }
    Candidates.Sort([](const auto& A, const auto& B) { return A.Key > B.Key; });
    for (const auto& Candidate : Candidates)
        if (PlanTo(Candidate.Value))
        {
            SetSearchState(ETrinitySearchState::Patrolling);
            return;
        }
    BeginFeeling();
}

void AHumanityTrinityRebuildSeeker::InvestigateSound()
{
    PlannedNoiseCount = HeardCount;
    ReplanAt = GetWorld()->GetTimeSeconds() + 1.f;
    SearchOrigin = LastHeard;
    LocalSearchesLeft = 2;
    // Sound uncertainty can put its estimated origin inside a desk. Project onto
    // the closest reachable floor node instead of requiring a path from inside it.
    int32 Goal = INDEX_NONE;
    float BestDistance = FLT_MAX;
    for (int32 I = 0; I < Nodes.Num(); ++I)
    {
        const float Distance = FVector::DistSquared2D(LastHeard, Nodes[I].Position);
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Goal = I;
        }
    }
    if (Goal != INDEX_NONE && PlanTo(Goal))
        SetSearchState(ETrinitySearchState::Investigating);
    else
        BeginFeeling();
}

void AHumanityTrinityRebuildSeeker::BeginFeeling()
{
    UE_LOG(LogTemp, Display, TEXT("[NPC_SEEKER] CHECK_AREA position=%s"), *GetActorLocation().ToString());
    Route.Empty();
    SetSearchState(ETrinitySearchState::Feeling);
    FeelingStartedAt = GetWorld()->GetTimeSeconds();
    PauseUntil = FeelingStartedAt + 4.2f;
    ++StopsSearched;
    for (int32 I = 0; I < Nodes.Num(); ++I)
        if (FVector::Dist2D(GetActorLocation(), Nodes[I].Position) < 135)
            LastVisited[I] = FeelingStartedAt;
}

void AHumanityTrinityRebuildSeeker::FinishFeeling()
{
    if (LocalSearchesLeft > 0)
    {
        --LocalSearchesLeft;
        int32 Best = INDEX_NONE;
        float Score = -FLT_MAX;
        for (int32 I = 0; I < Nodes.Num(); ++I)
        {
            const float Distance = FVector::Dist2D(SearchOrigin, Nodes[I].Position);
            const float Candidate = -LastVisited[I] + Nodes[I].Cover * 10;
            if (Distance < 220 && FVector::Dist2D(GetActorLocation(), Nodes[I].Position) > 85 && Candidate > Score)
            {
                Best = I;
                Score = Candidate;
            }
        }
        if (Best != INDEX_NONE && PlanTo(Best))
        {
            SetSearchState(ETrinitySearchState::Investigating);
            return;
        }
    }
    ChoosePatrolStop();
}

bool AHumanityTrinityRebuildSeeker::FeelAhead()
{
    if (!bSearching || !Seeker)
        return false;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(SeekerHandContact), true, this);
    const float Floor = GetActorLocation().Z - Body->GetUnscaledCapsuleHalfHeight();
    // Short physical sweeps at torso and crouch height. A desk or wall blocks the hand.
    for (float Height : {104.f, 62.f})
        for (float Yaw : {-32.f, 0.f, 32.f})
        {
            const FVector Start(GetActorLocation().X, GetActorLocation().Y, Floor + Height);
            const FVector End = Start + FRotator(0, GetActorRotation().Yaw + Yaw, 0).Vector() * 92.f;
            FHitResult Hit;
            if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
                    FCollisionShape::MakeSphere(10), Q) && Hit.GetActor() == Seeker)
            {
                if (auto* Game = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildHideAndSeekGameMode>())
                    Game->NotifyPlayerCaught(this);
                return true;
            }
        }
    return false;
}

void AHumanityTrinityRebuildSeeker::Tick(float Dt)
{
    AActor::Tick(Dt);
    const FVector Before = GetActorLocation();
    const float Now = GetWorld()->GetTimeSeconds();
    if (bSearching)
    {
        if (bHasNoiseMemory && HeardCount != PlannedNoiseCount && Now >= NextReactionAt && Now < MemoryUntil &&
            (SearchState == ETrinitySearchState::Patrolling || SearchState == ETrinitySearchState::Feeling))
        {
            SetSearchState(ETrinitySearchState::Listening);
            PauseUntil = Now + .65f;
            NextReactionAt = Now + 1.6f;
        }
        if (Now >= NextTouchAt)
        {
            NextTouchAt = Now + .22f;
            FeelAhead();
        }
        if (bSearching && SearchState == ETrinitySearchState::Listening && Now >= PauseUntil)
            InvestigateSound();
        if (bSearching && SearchState == ETrinitySearchState::Feeling)
        {
            AddActorWorldRotation(FRotator(0, Dt * 88.f, 0));
            if (Now >= PauseUntil)
                FinishFeeling();
        }
        if (bSearching && (SearchState == ETrinitySearchState::Patrolling ||
                           SearchState == ETrinitySearchState::Investigating))
        {
            if (SearchState == ETrinitySearchState::Investigating &&
                HeardCount != PlannedNoiseCount && Now >= ReplanAt && Now <= MemoryUntil)
                InvestigateSound();
            if (RouteCursor >= Route.Num())
                BeginFeeling();
            else if (CrouchAlpha < .15f)
            {
                FVector Target = Nodes[Route[RouteCursor]].Position;
                Target.Z = GetActorLocation().Z;
                const FVector Delta = Target - GetActorLocation();
                if (Delta.Size2D() < 5)
                    ++RouteCursor;
                else
                {
                    const float Speed = SearchState == ETrinitySearchState::Investigating ? 110.f : 85.f;
                    SetActorRotation(FMath::RInterpTo(GetActorRotation(), Delta.Rotation(), Dt, 6));
                    FHitResult Hit;
                    SetActorLocation(Before + Delta.GetSafeNormal() * FMath::Min(Speed * Dt, Delta.Size2D()), true, &Hit);
                    BlockedSeconds = Hit.bBlockingHit ? BlockedSeconds + Dt : 0;
                    if (BlockedSeconds > .8f)
                    {
                        UE_LOG(LogTemp, Display, TEXT("[NPC_SEEKER] BLOCKED replanning_at=%s"), *GetActorLocation().ToString());
                        AvoidNode = Route[RouteCursor];
                        AvoidUntil = Now + 10.f;
                        BeginFeeling();
                    }
                }
            }
        }
    }
    DistanceTravelled += FVector::Dist2D(Before, GetActorLocation());
    UpdateBodyAndFootsteps(Dt, Before);
}

bool AHumanityTrinityRebuildSeeker::WantsCrouch() const
{
    return SearchState == ETrinitySearchState::Feeling &&
           GetWorld()->GetTimeSeconds() - FeelingStartedAt > 1.8f;
}

float AHumanityTrinityRebuildSeeker::GetReachPose() const
{
    return bSearching ? (SearchState == ETrinitySearchState::Feeling ? 1.f : .55f) : 0.f;
}

FString AHumanityTrinityRebuildSeeker::GetSearchLabel() const
{
    switch (SearchState)
    {
    case ETrinitySearchState::Waiting: return TEXT("NPC is counting / cannot hear you yet");
    case ETrinitySearchState::Patrolling: return TEXT("NPC is checking hiding places");
    case ETrinitySearchState::Listening: return TEXT("NPC heard a sound / listening");
    case ETrinitySearchState::Investigating: return TEXT("NPC is checking the last sound");
    case ETrinitySearchState::Feeling: return TEXT("NPC is feeling around nearby furniture");
    default: return TEXT("Round over");
    }
}

void AHumanityTrinityRebuildSeeker::SetSearchState(ETrinitySearchState NewState)
{
    if (SearchState != NewState)
        UE_LOG(LogTemp, Display, TEXT("[NPC_SEEKER] STATE %d -> %d"), int32(SearchState), int32(NewState));
    SearchState = NewState;
}
