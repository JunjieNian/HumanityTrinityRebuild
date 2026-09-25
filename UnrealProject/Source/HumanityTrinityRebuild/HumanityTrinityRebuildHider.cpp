#include "HumanityTrinityRebuildHider.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"

namespace
{
const TCHAR* PartNames[] = {TEXT("Pelvis"), TEXT("Torso"),    TEXT("Head"),    TEXT("UpperArm"), TEXT("Forearm"),
                            TEXT("Hand"),   TEXT("UpperArm"), TEXT("Forearm"), TEXT("Hand"),     TEXT("Thigh"),
                            TEXT("Shin"),   TEXT("Shoe"),     TEXT("Thigh"),   TEXT("Shin"),     TEXT("Shoe")};
}

AHumanityTrinityRebuildHider::AHumanityTrinityRebuildHider()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HiderBody"));
    Body->InitCapsuleSize(24, 88);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Body->SetCollisionResponseToAllChannels(ECR_Block);
    // Hands touch the posed meshes instead of the empty space in a capsule.
    Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    RootComponent = Body;
    for (int32 i = 0; i < 15; ++i)
    {
        auto* Part = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Part_%02d"), i));
        Part->SetupAttachment(Body);
        Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Part->SetCollisionResponseToAllChannels(ECR_Ignore);
        Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        Parts.Add(Part);
    }
}
void AHumanityTrinityRebuildHider::BeginPlay()
{
    Super::BeginPlay();
    for (int32 i = 0; i < Parts.Num(); ++i)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/HumanityTrinityRebuild/Characters/Hider/SM_Hider_%s.SM_Hider_%s"), PartNames[i], PartNames[i]);
        Parts[i]->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *Path));
    }
    FootstepSound =
        LoadObject<USoundWave>(nullptr, TEXT("/Game/HumanityTrinityRebuild/Audio/SW_HiderFootstep.SW_HiderFootstep"));
    FootstepAttenuation = NewObject<USoundAttenuation>(this);
    auto& S = FootstepAttenuation->Attenuation;
    S.bAttenuate = true;
    S.bSpatialize = true;
    S.AttenuationShape = EAttenuationShape::Sphere;
    S.AttenuationShapeExtents = FVector(100, 0, 0);
    S.FalloffDistance = 1000;
    S.bEnableOcclusion = true;
    S.OcclusionTraceChannel = ECC_Visibility;
    S.OcclusionVolumeAttenuation = .3f;
    Animate(0, 0);
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] HIDER_READY parts=%d/15 audio=%s"), GetLoadedPartCount(),
           FootstepSound ? TEXT("YES") : TEXT("NO"));
}
int32 AHumanityTrinityRebuildHider::GetLoadedPartCount() const
{
    int32 Count = 0;
    for (const auto& Part : Parts)
        Count += Part && Part->GetStaticMesh() != nullptr;
    return Count;
}
void AHumanityTrinityRebuildHider::SetSeeker(AActor* InSeeker)
{
    Seeker = InSeeker;
    if (Seeker)
    {
        BuildRoutes();
        if (!Covers.IsEmpty())
        {
            const int32 Pick = Covers[FMath::RandRange(0, Covers.Num() - 1)];
            SetActorLocation(Nodes[Pick].Position);
            RecentCovers.Add(Pick);
        }
        SetState(EHiderState::Hidden);
    }
    else
        SetState(EHiderState::Caught);
}
bool AHumanityTrinityRebuildHider::IsPassageClear(const FVector& A, const FVector& B) const
{
    FCollisionQueryParams Q(SCENE_QUERY_STAT(HiderPassage), false, this);
    if (Seeker)
        Q.AddIgnoredActor(Seeker);
    FHitResult Hit;
    return !GetWorld()->SweepSingleByChannel(Hit, A, B, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(24, 88),
                                             Q);
}
void AHumanityTrinityRebuildHider::BuildRoutes(bool bWholeClassroom)
{
    // Flood only the classroom floor reachable by a standing person. Stage steps,
    // tables and prop-room doors are excluded from hiding destinations in this version.
    Nodes.Empty();
    Covers.Empty();
    Route.Empty();
    RecentCovers.Empty();
    const int32 W = bWholeClassroom ? 29 : 27, H = bWholeClassroom ? 33 : 29;
    auto Pos = [W, bWholeClassroom](int32 I) {
        return FVector((bWholeClassroom ? -560 : -520) + (I % W) * 40,
                       (bWholeClassroom ? -80 : -220) - (I / W) * 40, 90);
    };
    TArray<int32> Map;
    Map.Init(INDEX_NONE, W * H);
    TArray<int32> Queue;
    const int32 Seed = W / 2 + (bWholeClassroom ? 4 * W : 0);
    Queue.Add(Seed);
    Map[Seed] = 0;
    Nodes.Add({Pos(Seed), {}, 0});
    for (int32 Cursor = 0; Cursor < Queue.Num(); ++Cursor)
    {
        int32 Cell = Queue[Cursor], Current = Map[Cell];
        for (const FIntPoint D : {FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1)})
        {
            int32 X = Cell % W + D.X, Y = Cell / W + D.Y;
            if (X < 0 || X >= W || Y < 0 || Y >= H)
                continue;
            int32 Next = Y * W + X;
            if (!IsPassageClear(Pos(Cell), Pos(Next)))
                continue;
            if (Map[Next] == INDEX_NONE)
            {
                Map[Next] = Nodes.Num();
                Nodes.Add({Pos(Next), {}, 0});
                Queue.Add(Next);
            }
            Nodes[Current].Neighbors.AddUnique(Map[Next]);
        }
    }
    FCollisionQueryParams Q(SCENE_QUERY_STAT(HiderCover), true, this);
    if (Seeker)
        Q.AddIgnoredActor(Seeker);
    TArray<int32> Ranked;
    for (int32 I = 0; I < Nodes.Num(); ++I)
    {
        const FVector P = Nodes[I].Position;
        if (P.Y > -460 || P.Y < -1300)
            continue;
        for (int32 Ray = 0; Ray < 8; ++Ray)
        {
            const float A = Ray * PI / 4;
            FHitResult Hit;
            FVector From(P.X, P.Y, 58);
            if (GetWorld()->LineTraceSingleByChannel(Hit, From, From + FVector(FMath::Cos(A), FMath::Sin(A), 0) * 145,
                                                     ECC_Visibility, Q))
                Nodes[I].Cover += 1.f - Hit.Distance / 220.f;
        }
        if (Nodes[I].Cover > .5f)
            Ranked.Add(I);
    }
    Ranked.Sort([this](int32 A, int32 B) { return Nodes[A].Cover > Nodes[B].Cover; });
    for (int32 I : Ranked)
    {
        bool bSpaced = true;
        for (int32 Other : Covers)
            if (FVector::Dist2D(Nodes[I].Position, Nodes[Other].Position) < 210)
            {
                bSpaced = false;
                break;
            }
        if (bSpaced)
            Covers.Add(I);
        if (Covers.Num() >= 12)
            break;
    }
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] NAVIGATION nodes=%d covers=%d"), Nodes.Num(), Covers.Num());
}
int32 AHumanityTrinityRebuildHider::NearestNode(const FVector& P) const
{
    int32 Result = INDEX_NONE;
    float Best = FLT_MAX;
    FVector Flat = P;
    Flat.Z = 90;
    for (int32 I = 0; I < Nodes.Num(); ++I)
    {
        const float D = FVector::DistSquared2D(P, Nodes[I].Position);
        if (D < Best && IsPassageClear(Flat, Nodes[I].Position))
        {
            Best = D;
            Result = I;
        }
    }
    return Result;
}
void AHumanityTrinityRebuildHider::HearNoise(const FVector& Location, float Loudness)
{
    if (!Seeker || ShowcasePose >= 0 || State == EHiderState::Caught)
        return;
    const float Distance = FVector::Dist2D(Location, GetActorLocation());
    float Range = FMath::Clamp(Loudness, 0.f, 1.f) * 700.f;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(HiderHearing), true, this);
    Q.AddIgnoredActor(Seeker);
    FHitResult Hit;
    const FVector Ear = GetActorLocation() + FVector(0, 0, 30);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Location + FVector(0, 0, 30), Ear, ECC_Visibility, Q))
        Range *= .5f;
    if (Distance > Range || Range <= 0)
        return;
    LastHeard = Location;
    MemoryUntil = GetWorld()->GetTimeSeconds() + 7.f;
    bHasNoiseMemory = true;
    ++HeardCount;
    if (State == EHiderState::Hidden)
    {
        SetState(EHiderState::Listening);
        ReactionAt = GetWorld()->GetTimeSeconds() + FMath::FRandRange(.7f, 1.15f);
    }
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] HEARD distance=%.0f loudness=%.2f state=%s"), Distance, Loudness,
           *GetBehaviorLabel());
}
bool AHumanityTrinityRebuildHider::ChooseEscape()
{
    const int32 Start = NearestNode(GetActorLocation());
    if (Start == INDEX_NONE || Covers.Num() < 2)
        return false;
    TArray<float> Costs;
    Costs.Init(FLT_MAX, Nodes.Num());
    Costs[Start] = 0;
    TArray<int32> Parent;
    Parent.Init(INDEX_NONE, Nodes.Num());
    TArray<bool> Done;
    Done.Init(false, Nodes.Num());
    // Penalize travel near the remembered sound, encouraging a route around furniture.
    for (int32 Iter = 0; Iter < Nodes.Num(); ++Iter)
    {
        int32 Current = INDEX_NONE;
        float Best = FLT_MAX;
        for (int32 I = 0; I < Nodes.Num(); ++I)
            if (!Done[I] && Costs[I] < Best)
            {
                Current = I;
                Best = Costs[I];
            }
        if (Current == INDEX_NONE)
            break;
        Done[Current] = true;
        for (int32 Next : Nodes[Current].Neighbors)
        {
            float Danger = FMath::Max(0.f, 350.f - FVector::Dist2D(Nodes[Next].Position, LastHeard));
            float Cost = Costs[Current] + 40 + Danger * .9f;
            if (Cost < Costs[Next])
            {
                Costs[Next] = Cost;
                Parent[Next] = Current;
            }
        }
    }
    int32 Goal = INDEX_NONE;
    float BestScore = -FLT_MAX;
    for (int32 C : Covers)
    {
        if (Costs[C] == FLT_MAX || FVector::Dist2D(Nodes[C].Position, GetActorLocation()) < 200)
            continue;
        const float ThreatDistance = FVector::Dist2D(Nodes[C].Position, LastHeard);
        float Score = FMath::Min(ThreatDistance, 850.f) * 1.4f + Nodes[C].Cover * 65 - Costs[C] * .22f;
        if (RecentCovers.Contains(C))
            Score -= 400;
        if (Score > BestScore)
        {
            BestScore = Score;
            Goal = C;
        }
    }
    if (Goal == INDEX_NONE)
        return false;
    Route.Empty();
    for (int32 N = Goal; N != INDEX_NONE; N = Parent[N])
        Route.Insert(N, 0);
    RouteCursor = 0;
    RecentCovers.Add(Goal);
    if (RecentCovers.Num() > 3)
        RecentCovers.RemoveAt(0);
    BlockedSeconds = 0;
    ReplanAt = GetWorld()->GetTimeSeconds() + 3.f;
    PlannedNoiseCount = HeardCount;
    SetState(EHiderState::Relocating);
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] ESCAPE route_nodes=%d goal=%s"), Route.Num(),
           *Nodes[Goal].Position.ToString());
    return true;
}
FString AHumanityTrinityRebuildHider::GetBehaviorLabel() const
{
    switch (State)
    {
    case EHiderState::Hidden:
        return TEXT("Crouching in cover");
    case EHiderState::Listening:
        return TEXT("Listening");
    case EHiderState::Relocating:
        return TEXT("Moving to another hiding place");
    default:
        return TEXT("Found");
    }
}
void AHumanityTrinityRebuildHider::SetState(EHiderState NewState)
{
    if (State != NewState)
        UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] BEHAVIOR %d -> %d"), int32(State), int32(NewState));
    State = NewState;
}
void AHumanityTrinityRebuildHider::SetShowcasePose(int32 Pose)
{
    ShowcasePose = Pose;
}
void AHumanityTrinityRebuildHider::Tick(float Dt)
{
    Super::Tick(Dt);
    const FVector Before = GetActorLocation();
    const float Now = GetWorld()->GetTimeSeconds();
    if (Seeker && ShowcasePose < 0)
    {
        if (State == EHiderState::Listening && Now >= ReactionAt)
        {
            // A remote sound warrants listening, but not necessarily revealing oneself.
            if (!bHasNoiseMemory || Now > MemoryUntil || FVector::Dist2D(LastHeard, Before) > 420 || !ChooseEscape())
                SetState(EHiderState::Hidden);
        }
        if (State == EHiderState::Relocating)
        {
            if (HeardCount > PlannedNoiseCount && Now >= ReplanAt && Now < MemoryUntil &&
                FVector::Dist2D(LastHeard, GetActorLocation()) < 300)
                ChooseEscape();
            if (RouteCursor >= Route.Num())
            {
                SetState(EHiderState::Hidden);
                bHasNoiseMemory = false;
            }
            else
            {
                FVector Target = Nodes[Route[RouteCursor]].Position;
                Target.Z = GetActorLocation().Z;
                FVector Delta = Target - GetActorLocation();
                Delta.Z = 0;
                if (Delta.Size2D() < 7)
                    ++RouteCursor;
                else if (CrouchAlpha < .15f)
                {
                    FHitResult Hit;
                    SetActorRotation(FMath::RInterpTo(GetActorRotation(), Delta.Rotation(), Dt, 7));
                    SetActorLocation(GetActorLocation() + Delta.GetSafeNormal() * FMath::Min(95.f * Dt, Delta.Size2D()),
                                     true, &Hit);
                    BlockedSeconds = Hit.bBlockingHit ? BlockedSeconds + Dt : 0;
                    if (BlockedSeconds > .45f)
                    {
                        SetState(EHiderState::Listening);
                        ReactionAt = Now + 1.2f;
                        // A live sweep still prevents movement through a player on the static graph.
                        if (Now < ReplanAt)
                        {
                            SetState(EHiderState::Hidden);
                            bHasNoiseMemory = false;
                        }
                    }
                }
            }
        }
    }
    UpdateBodyAndFootsteps(Dt, Before);
}
void AHumanityTrinityRebuildHider::UpdateBodyAndFootsteps(float Dt, const FVector& Before)
{
    const float Distance = FVector::Dist2D(Before, GetActorLocation());
    Animate(Dt, Distance);
    if (Distance > 0.01f)
    {
        DistanceSinceStep += Distance;
        if (DistanceSinceStep >= 42)
        {
            DistanceSinceStep = FMath::Fmod(DistanceSinceStep, 42.f);
            ++StepsEmitted;
            if (FootstepSound)
                UGameplayStatics::PlaySoundAtLocation(this, FootstepSound, GetActorLocation() - FVector(0, 0, 60), .8f,
                                                      FMath::FRandRange(.94f, 1.06f), 0, FootstepAttenuation);
        }
    }
}
bool AHumanityTrinityRebuildHider::WantsCrouch() const
{
    return ShowcasePose >= 0 ? ShowcasePose == 1 : State == EHiderState::Hidden || State == EHiderState::Listening;
}
void AHumanityTrinityRebuildHider::Animate(float Dt, float Distance)
{
    const bool bCrouch = WantsCrouch();
    CrouchAlpha = FMath::FInterpTo(CrouchAlpha, bCrouch ? 1.f : 0.f, Dt, 7.f);
    const float Half = 88 - CrouchAlpha * 20;
    Body->SetCapsuleHalfHeight(Half, false);
    FVector P = GetActorLocation();
    P.Z = Half + 2;
    SetActorLocation(P, false);
    const float Speed = Dt > 0 ? Distance / Dt : 0;
    WalkAlpha = FMath::FInterpTo(WalkAlpha, (Speed > 5 || ShowcasePose == 2) ? 1.f : 0.f, Dt, 9);
    PoseTime += Dt;
    const float Phase = ShowcasePose == 2 ? 1.f : PoseTime * 7.f;
    const float Hips = 89 - CrouchAlpha * 32;
    auto Pose = [&](int32 I, FVector Pos, FQuat Rot = FQuat::Identity) {
        Pos.Z -= Half;
        Parts[I]->SetRelativeLocationAndRotation(Pos, Rot);
    };
    const float Breath = FMath::Sin(PoseTime * 2) * .35f;
    Pose(0, FVector(0, 0, Hips));
    const FQuat Lean = FRotator(-CrouchAlpha * 10, 0, 0).Quaternion();
    Pose(1, FVector(0, 0, Hips + Breath), Lean);
    FVector Neck = FVector(0, 0, Hips) + Lean.RotateVector(FVector(0, 0, 55.5f));
    Pose(2, Neck, FRotator(0, State == EHiderState::Listening ? FMath::Sin(PoseTime * 2) * 18 : 0, 0).Quaternion());
    for (int32 Side = 0; Side < 2; ++Side)
    {
        const float Sign = Side == 0 ? 1.f : -1.f;
        const float Swing = FMath::Sin(Phase + Side * PI) * WalkAlpha;
        FVector Hip(0, Sign * 9.9f, Hips);
        FVector Ankle(Swing * 16, Sign * 9.9f, 6.5f + FMath::Max(0.f, FMath::Cos(Phase + Side * PI)) * 7 * WalkAlpha);
        FVector Direction = (Ankle - Hip).GetSafeNormal();
        const float D = FMath::Clamp((Ankle - Hip).Size(), 1.f, 82.9f), L1 = 42, L2 = 41;
        const float Along = (L1 * L1 - L2 * L2 + D * D) / (2 * D);
        const float Bend = FMath::Sqrt(FMath::Max(0.f, L1 * L1 - Along * Along));
        FVector Front = (FVector::ForwardVector - Direction * FVector::DotProduct(FVector::ForwardVector, Direction))
                            .GetSafeNormal();
        const FVector Knee = Hip + Direction * Along + Front * Bend;
        Pose(9 + Side * 3, Hip, FQuat::FindBetweenNormals(FVector(0, 0, -1), (Knee - Hip).GetSafeNormal()));
        Pose(10 + Side * 3, Knee, FQuat::FindBetweenNormals(FVector(0, 0, -1), (Ankle - Knee).GetSafeNormal()));
        Pose(11 + Side * 3, Ankle);
        const FVector Shoulder = FVector(0, 0, Hips) + Lean.RotateVector(FVector(0, Sign * 21.9f, 41.5f));
        const float Reach = GetReachPose();
        const FQuat Arm = FRotator(FMath::Lerp(Swing * 19 + CrouchAlpha * 32, 72.f, Reach), 0, Sign * 3).Quaternion();
        const FQuat Fore = FRotator(FMath::Lerp(Swing * 12 + CrouchAlpha * 72 + 8, 88.f, Reach), 0, 0).Quaternion();
        const FVector Elbow = Shoulder + Arm.RotateVector(FVector(0, 0, -28));
        Pose(3 + Side * 3, Shoulder, Arm);
        Pose(4 + Side * 3, Elbow, Fore);
        Pose(5 + Side * 3, Elbow + Fore.RotateVector(FVector(0, 0, -27)), Fore);
    }
}
