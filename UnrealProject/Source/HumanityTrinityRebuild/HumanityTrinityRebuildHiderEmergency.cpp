#include "HumanityTrinityRebuildHider.h"
#include "HumanityTrinityRebuildRoomInteraction.h"
#include "HumanityTrinityRebuildDoorLayout.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"

bool AHumanityTrinityRebuildHider::BeginEmergencyHide(AHumanityTrinityRebuildRoomInteraction* Room, int32 DoorIndex)
{
    if (bEmergencyActive || !Room || DoorIndex < 0 || DoorIndex > 1)
        return false;
    EmergencyRoom = Room;
    EmergencyDoorIndex = DoorIndex;
    EmergencyStartedAt = GetWorld()->GetTimeSeconds();
    EmergencyFloorHeight = FMath::Max(0.f, GetActorLocation().Z - Body->GetUnscaledCapsuleHalfHeight() - 2.f);
    if (Nodes.IsEmpty())
        BuildRoutes(true);
    if (!BuildEmergencyPath())
    {
        UE_LOG(LogTemp, Warning, TEXT("[TEACHER_PATROL] NPC_NO_ESCAPE_ROUTE actor=%s"), *GetName());
        return false;
    }
    bEmergencyActive = true;
    bEmergencyHolding = bEmergencyReturning = false;
    EmergencyCursor = 0;
    EmergencyBlocked = 0;
    // Curtains are a passage obstacle, so the NPC visibly opens them before using the stairs.
    Room->SetCurtainsOpen(true);
    UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL] NPC_ESCAPE actor=%s room=%d route=%d start=%s"),
        *GetName(), DoorIndex, EmergencyPath.Num(), *GetActorLocation().ToString());
    return true;
}

bool AHumanityTrinityRebuildHider::BuildEmergencyPath()
{
    const int32 Start = NearestNode(GetActorLocation());
    if (Start == INDEX_NONE)
        return false;
    const float Side = EmergencyDoorIndex == 0 ? 1.f : -1.f;
    const FVector StairApproach(Side * 180, -1330, 90);
    TArray<int32> Parent, Queue;
    Parent.Init(INDEX_NONE, Nodes.Num());
    Parent[Start] = Start;
    Queue.Add(Start);
    int32 Goal = Start;
    float Best = FVector::DistSquared2D(Nodes[Start].Position, StairApproach);
    for (int32 Cursor = 0; Cursor < Queue.Num(); ++Cursor)
    {
        const int32 Current = Queue[Cursor];
        const float Distance = FVector::DistSquared2D(Nodes[Current].Position, StairApproach);
        if (Distance < Best)
        {
            Goal = Current;
            Best = Distance;
        }
        for (int32 Next : Nodes[Current].Neighbors)
            if (Parent[Next] == INDEX_NONE)
            {
                Parent[Next] = Current;
                Queue.Add(Next);
            }
    }
    if (Best > FMath::Square(140.f))
        return false;
    EmergencyPath.Reset();
    for (int32 N = Goal;; N = Parent[N])
    {
        EmergencyPath.Insert(Nodes[N].Position, 0);
        if (N == Start)
            break;
    }
    EmergencyPath.Insert(GetActorLocation(), 0);
    EmergencyPath.Add(FVector(Side * 180, -1340, 90));
    EmergencyPath.Add(FVector(Side * 180, -1410, 111));
    EmergencyPath.Add(FVector(Side * 180, -1490, 132));
    const auto& Door = HumanityPropDoors[EmergencyDoorIndex];
    const FVector Centre = Door.Hinge + FRotator(0, Door.Yaw, 0).RotateVector(FVector(Door.Width * .5f, 0, 0));
    EmergencyPath.Add(Centre + Door.StageNormal * 100 + FVector(0, 0, 90));
    EmergencyPath.Add(Centre + Door.StageNormal * 65 + FVector(0, 0, 90));
    // Clear the complete 95 cm door sweep before crouching and closing it.
    EmergencyPath.Add(Centre - Door.StageNormal * 155 + FVector(0, 0, 50));
    return true;
}

void AHumanityTrinityRebuildHider::EndEmergencyHide()
{
    if (!bEmergencyActive || bEmergencyReturning)
        return;
    // Reverse only the route actually travelled. A disabled event does not make
    // an NPC jump past furniture, doors, or the step down from the stage.
    const int32 Last = FMath::Clamp(EmergencyCursor - 1, 0, EmergencyPath.Num() - 1);
    EmergencyPath.SetNum(Last + 1);
    if (bEmergencyHolding)
        EmergencyPath.Last() = GetActorLocation();
    for (int32 A = 0, B = EmergencyPath.Num() - 1; A < B; ++A, --B)
        EmergencyPath.Swap(A, B);
    EmergencyCursor = 0;
    bEmergencyHolding = false;
    bEmergencyReturning = true;
    if (EmergencyRoom)
        EmergencyRoom->SetPropDoorOpen(EmergencyDoorIndex, true);
}

bool AHumanityTrinityRebuildHider::MoveEmergencyStep(const FVector& Delta)
{
    const FVector Before = GetActorLocation();
    FHitResult Hit;
    SetActorLocation(Before + Delta, true, &Hit);
    if (Hit.bBlockingHit && FMath::Abs(Hit.ImpactNormal.Z) < .4f)
    {
        // The classroom and prop-room thresholds are 21 cm steps. A three-part
        // capsule sweep implements real step-up and never crosses a solid wall.
        const FVector AtBlock = GetActorLocation();
        FHitResult Up, Across;
        SetActorLocation(AtBlock + FVector(0, 0, 24), true, &Up);
        if (!Up.bBlockingHit)
            SetActorLocation(GetActorLocation() + Delta * (1.f - Hit.Time), true, &Across);
        FHitResult Down;
        SetActorLocation(GetActorLocation() - FVector(0, 0, 24), true, &Down);
    }
    // Settle onto the floor after descending into either dark room. Root motion
    // remains swept, with collision active throughout the complete route.
    FHitResult Floor;
    SetActorLocation(GetActorLocation() - FVector(0, 0, 48), true, &Floor);
    EmergencyFloorHeight = FMath::Max(0.f, GetActorLocation().Z - Body->GetUnscaledCapsuleHalfHeight() - 2.f);
    return FVector::Dist2D(Before, GetActorLocation()) > .05f;
}

bool AHumanityTrinityRebuildHider::TickEmergency(float Dt)
{
    if (!bEmergencyActive)
        return false;
    const FVector Before = GetActorLocation();
    if (!EmergencyRoom)
    {
        EndEmergencyHide();
        UpdateBodyAndFootsteps(Dt, Before);
        return true;
    }
    if (bEmergencyHolding)
    {
        UpdateBodyAndFootsteps(Dt, Before);
        return true;
    }
    if (EmergencyCursor >= EmergencyPath.Num())
    {
        if (bEmergencyReturning)
        {
            const float Paused = GetWorld()->GetTimeSeconds() - EmergencyStartedAt;
            bEmergencyActive = bEmergencyReturning = false;
            EmergencyRoom->SetPropDoorOpen(EmergencyDoorIndex, false);
            MemoryUntil += Paused;
            ReactionAt += Paused;
            ReplanAt += Paused;
            EmergencyFinished(Paused);
            UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL] NPC_RESUMED actor=%s position=%s"), *GetName(), *GetActorLocation().ToString());
        }
        else
        {
            bEmergencyHolding = true;
            EmergencyRoom->SetPropDoorOpen(EmergencyDoorIndex, false);
            UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL] NPC_HIDDEN actor=%s room=%d position=%s"), *GetName(), EmergencyDoorIndex, *GetActorLocation().ToString());
        }
        UpdateBodyAndFootsteps(Dt, Before);
        return true;
    }
    const auto& Door = HumanityPropDoors[EmergencyDoorIndex];
    const FVector Centre = Door.Hinge + FRotator(0, Door.Yaw, 0).RotateVector(FVector(Door.Width * .5f, 0, 0));
    if (FVector::Dist2D(GetActorLocation(), Centre) < 180)
    {
        EmergencyRoom->SetPropDoorOpen(EmergencyDoorIndex, true);
        // Wait outside the swing arc instead of wedging between a moving leaf
        // and its visitor-overlap guard, on both entry and the return journey.
        if (EmergencyRoom->GetPropDoorOpenFraction(EmergencyDoorIndex) < .98f)
        {
            UpdateBodyAndFootsteps(Dt, Before);
            return true;
        }
    }
    FVector Delta = EmergencyPath[EmergencyCursor] - GetActorLocation();
    Delta.Z = 0;
    if (Delta.Size2D() < 6)
        ++EmergencyCursor;
    else if (CrouchAlpha < .15f)
    {
        SetActorRotation(FMath::RInterpTo(GetActorRotation(), Delta.Rotation(), Dt, 8));
        // Substeps keep capsule step-up robust during slower capture frames.
        float Distance = FMath::Min(235.f * FMath::Min(Dt, .1f), Delta.Size2D());
        bool bMoved = false;
        while (Distance > .01f)
        {
            const float Step = FMath::Min(8.f, Distance);
            bMoved |= MoveEmergencyStep(Delta.GetSafeNormal() * Step);
            Distance -= Step;
        }
        EmergencyBlocked = bMoved ? 0 : EmergencyBlocked + Dt;
        if (!bEmergencyReturning && EmergencyCursor >= EmergencyPath.Num() - 1 &&
            FVector::DotProduct(GetActorLocation() - Centre, Door.StageNormal) < -140)
            EmergencyCursor = EmergencyPath.Num();
        if (EmergencyBlocked > 2.f && EmergencyPath.IsValidIndex(EmergencyCursor))
        {
            UE_LOG(LogTemp, Warning, TEXT("[TEACHER_PATROL] NPC_ROUTE_BLOCKED actor=%s cursor=%d at=%s target=%s"),
                *GetName(), EmergencyCursor, *GetActorLocation().ToString(), *EmergencyPath[EmergencyCursor].ToString());
            EmergencyBlocked = 0;
        }
    }
    UpdateBodyAndFootsteps(Dt, Before);
    return true;
}
