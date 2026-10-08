#include "HumanityTrinityRebuildTeacherPatrol.h"
#include "HumanityTrinityRebuildCorridor.h"
#include "HumanityTrinityRebuildTeacher.h"
#include "HumanityTrinityRebuildHider.h"
#include "HumanityTrinityRebuildGameMode.h"
#include "HumanityTrinityRebuildHideAndSeekGameMode.h"
#include "HumanityTrinityRebuildRoomInteraction.h"
#include "HumanityTrinityRebuildLightingController.h"
#include "HumanityTrinityRebuildDoorLayout.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"

namespace { bool bSessionTeacherPatrolEnabled = false; }

AHumanityTrinityRebuildTeacherPatrol::AHumanityTrinityRebuildTeacherPatrol()
{
    PrimaryActorTick.bCanEverTick = true;
}
void AHumanityTrinityRebuildTeacherPatrol::SetSessionEnabled(bool bInEnabled) { bSessionTeacherPatrolEnabled = bInEnabled; }
bool AHumanityTrinityRebuildTeacherPatrol::IsSessionEnabled() { return bSessionTeacherPatrolEnabled; }
AHumanityTrinityRebuildTeacherPatrol* AHumanityTrinityRebuildTeacherPatrol::Find(UWorld* World)
{
    if (World)
        for (TActorIterator<AHumanityTrinityRebuildTeacherPatrol> It(World); It; ++It)
            return *It;
    return nullptr;
}

void AHumanityTrinityRebuildTeacherPatrol::RefreshWorldActors()
{
    for (TActorIterator<AHumanityTrinityRebuildRoomInteraction> It(GetWorld()); It; ++It) { Room = *It; break; }
    for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It) { Lighting = *It; break; }
    for (TActorIterator<AHumanityTrinityRebuildCorridor> It(GetWorld()); It; ++It) { Corridor = *It; break; }
    if (!Corridor)
        Corridor = GetWorld()->SpawnActor<AHumanityTrinityRebuildCorridor>();
    if (!Teacher)
        Teacher = GetWorld()->SpawnActor<AHumanityTrinityRebuildTeacher>();
}

void AHumanityTrinityRebuildTeacherPatrol::BeginPlay()
{
    Super::BeginPlay();
    RefreshWorldActors();
    if (Teacher)
    {
        Teacher->SetActorHiddenInGame(true);
        Teacher->SetActorEnableCollision(false);
    }
    auto Load = [](const TCHAR* Name) {
        return LoadObject<USoundWave>(nullptr, *FString::Printf(TEXT("/Game/HumanityTrinityRebuild/Audio/%s.%s"), Name, Name));
    };
    Footsteps = Load(TEXT("SW_TeacherFootstep"));
    Keys = Load(TEXT("SW_TeacherKeys"));
    WarningVoice = Load(TEXT("SW_TeacherWarning"));
    ClearVoice = Load(TEXT("SW_TeacherClear"));
    CaughtVoice = Load(TEXT("SW_TeacherCaught"));
    AudioAttenuation = NewObject<USoundAttenuation>(this);
    auto& S = AudioAttenuation->Attenuation;
    S.bAttenuate = true;
    S.bSpatialize = true;
    S.AttenuationShape = EAttenuationShape::Sphere;
    S.AttenuationShapeExtents = FVector(700, 0, 0);
    S.FalloffDistance = 1800;
    S.bEnableOcclusion = true;
    S.OcclusionTraceChannel = ECC_Visibility;
    S.OcclusionVolumeAttenuation = .7f;
    S.OcclusionLowPassFilterFrequency = 2400;
    SetEnabled(IsSessionEnabled());
    UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL] READY enabled=%d audio=%d teacher_parts=%d"),
        bEnabled, HasAllAudio(), Teacher ? Teacher->GetLoadedPartCount() : 0);
}

bool AHumanityTrinityRebuildTeacherPatrol::HasAllAudio() const
{
    return Footsteps && Keys && WarningVoice && ClearVoice && CaughtVoice;
}
void AHumanityTrinityRebuildTeacherPatrol::SetEnabled(bool bInEnabled)
{
    bEnabled = bInEnabled;
    SetSessionEnabled(bInEnabled);
    if (!bEnabled && IsActive())
        CancelPatrol();
    NextPatrolAt = GetWorld()->GetTimeSeconds() + FMath::FRandRange(FirstDelayMin, FirstDelayMax);
    UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL] SET_ENABLED %d"), bEnabled);
}

bool AHumanityTrinityRebuildTeacherPatrol::StartPatrol()
{
    if (!bEnabled || IsActive())
        return false;
    auto* Game = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildGameMode>();
    if (Game && !Game->IsTeacherPatrolAllowed())
        return false;
    RefreshWorldActors();
    if (!Corridor || !Teacher || !Room || !Lighting)
        return false;
    if (Game)
        Game->SetTeacherPatrolSuspended(true);
    // The side-room escape must remain available even in walkthrough sessions
    // where the visitor previously closed the stage curtain.
    Room->SetCurtainsOpen(true);
    Participants.Reset();
    for (TActorIterator<AHumanityTrinityRebuildHider> It(GetWorld()); It; ++It)
    {
        Participants.Add(*It);
        // Stable side preference avoids two NPCs competing for one doorway.
        const int32 Side = It->GetActorLocation().X >= 0 ? 0 : 1;
        It->BeginEmergencyHide(Room, Side);
    }
    FailureReason.Empty();
    bExitRequested = false;
    bWaitForPlayerReturn = false;
    Corridor->SetPatrolDoorLocked(true);
    for (int32 I = 0; I < 3; ++I)
        Corridor->SetDoorOpen(I, false);
    Teacher->SetActorHiddenInGame(false);
    Teacher->SetActorLocation(Corridor->GetCorridorApproachLocation());
    Teacher->SetWalking(true);
    NextFootstepAt = GetWorld()->GetTimeSeconds();
    NextKeysAt = NextFootstepAt + 2.2f;
    ChangePhase(ETeacherPatrolPhase::Warning, WarningSeconds);
    PlayTeacherSound(WarningVoice, 1.15f);
    return true;
}

void AHumanityTrinityRebuildTeacherPatrol::ChangePhase(ETeacherPatrolPhase NewPhase, float Duration)
{
    Phase = NewPhase;
    PhaseStartedAt = GetWorld()->GetTimeSeconds();
    PhaseEndsAt = PhaseStartedAt + Duration;
    TeacherPhaseOrigin = Teacher ? Teacher->GetActorLocation() : FVector::ZeroVector;
    UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL] PHASE %d duration=%.2f"), int32(Phase), Duration);
}

void AHumanityTrinityRebuildTeacherPatrol::PlayTeacherSound(USoundWave* Sound, float Volume)
{
    if (Sound && Teacher)
        UGameplayStatics::PlaySoundAtLocation(this, Sound, Teacher->GetActorLocation() + FVector(0, 0, 140),
            Volume, 1.f, 0.f, AudioAttenuation);
}

bool AHumanityTrinityRebuildTeacherPatrol::IsActorSafelyHidden(const AActor* Actor) const
{
    if (!Actor || !Room)
        return false;
    const FVector P = Actor->GetActorLocation();
    if (P.Y > -1470 || P.Y < -1780 || FMath::Abs(P.X) > 574 || P.Z < 35 || P.Z > 220)
        return false;
    for (int32 I = 0; I < 2; ++I)
    {
        const auto& Door = HumanityPropDoors[I];
        const FVector Centre = Door.Hinge + FRotator(0, Door.Yaw, 0).RotateVector(FVector(Door.Width * .5f, 0, 0));
        const float BehindDoor = FVector::DotProduct(P - Centre, Door.StageNormal);
        if (BehindDoor < -42 && Room->GetPropDoorOpenFraction(I) < .03f)
            return true;
    }
    return false;
}

void AHumanityTrinityRebuildTeacherPatrol::CheckForDiscovery()
{
    if (Lighting && Lighting->AreMainLightsOn())
        Fail(TEXT("教室灯还亮着，老师发现了你们。"));
    else if (Room && Room->IsScreenIlluminating())
        Fail(TEXT("教学屏幕还亮着，老师发现了你们。"));
    else if (!IsActorSafelyHidden(UGameplayStatics::GetPlayerPawn(this, 0)))
        Fail(TEXT("你没有躲进两侧暗房并关好门，老师发现了你。"));
    else
        for (AHumanityTrinityRebuildHider* NPC : Participants)
            if (IsValid(NPC) && !IsActorSafelyHidden(NPC))
            {
                Fail(TEXT("还有同伴留在外面，老师发现了你们。"));
                break;
            }
}

void AHumanityTrinityRebuildTeacherPatrol::Fail(const FString& Reason)
{
    if (HasFailed())
        return;
    FailureReason = Reason;
    Teacher->SetWalking(false);
    Teacher->FaceIntoRoom();
    ChangePhase(ETeacherPatrolPhase::Failed, FailureExitSeconds);
    PlayTeacherSound(CaughtVoice, 1.2f);
    UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL] FAILED %s"), *Reason);
}

void AHumanityTrinityRebuildTeacherPatrol::CancelPatrol()
{
    if (!IsActive())
        return;
    if (Teacher)
        Teacher->SetActorHiddenInGame(true);
    if (Corridor)
    {
        for (int32 I = 0; I < 3; ++I)
            Corridor->SetDoorOpen(I, false);
        Corridor->SetPatrolDoorLocked(false);
    }
    for (AHumanityTrinityRebuildHider* NPC : Participants)
        if (IsValid(NPC))
            NPC->EndEmergencyHide();
    FailureReason.Empty();
    bWaitForPlayerReturn = false;
    ChangePhase(ETeacherPatrolPhase::Recovering, 0);
    if (Participants.IsEmpty())
        FinishPatrol();
}

void AHumanityTrinityRebuildTeacherPatrol::FinishPatrol()
{
    if (auto* Game = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildGameMode>())
        Game->SetTeacherPatrolSuspended(false);
    Corridor->SetPatrolDoorLocked(false);
    Teacher->SetActorHiddenInGame(true);
    Teacher->SetWalking(false);
    Participants.Reset();
    ChangePhase(ETeacherPatrolPhase::Dormant, 0);
    NextPatrolAt = GetWorld()->GetTimeSeconds() + FMath::FRandRange(RepeatDelayMin, RepeatDelayMax);
}

void AHumanityTrinityRebuildTeacherPatrol::Tick(float Dt)
{
    Super::Tick(Dt);
    const float Now = GetWorld()->GetTimeSeconds();
    if (Phase == ETeacherPatrolPhase::Dormant)
    {
        if (bEnabled && Now >= NextPatrolAt)
            StartPatrol();
        return;
    }
    if (Phase == ETeacherPatrolPhase::Warning || Phase == ETeacherPatrolPhase::Departing)
    {
        const bool bArriving = Phase == ETeacherPatrolPhase::Warning;
        const FVector Destination = bArriving ? Corridor->GetInspectionLocation() : Corridor->GetCorridorApproachLocation();
        const float Fraction = FMath::Clamp((Now - PhaseStartedAt) / FMath::Max(.1f, PhaseEndsAt - PhaseStartedAt), 0.f, 1.f);
        Teacher->SetActorLocation(FMath::Lerp(TeacherPhaseOrigin, Destination, Fraction));
        const FVector Direction = Destination - TeacherPhaseOrigin;
        Teacher->SetActorRotation(Direction.Rotation());
        if (Now >= NextFootstepAt)
        {
            PlayTeacherSound(Footsteps, bArriving ? .6f + Fraction * .55f : 1.f - Fraction * .6f);
            NextFootstepAt = Now + (bArriving ? .72f : .58f);
        }
        if (bArriving && Now >= NextKeysAt)
        {
            PlayTeacherSound(Keys, .5f + Fraction * .25f);
            NextKeysAt = Now + 5.f;
        }
        if (Now >= PhaseEndsAt)
        {
            if (bArriving)
            {
                Teacher->SetWalking(false);
                Teacher->FaceIntoRoom();
                Corridor->SetDoorOpen(1, true);
                ChangePhase(ETeacherPatrolPhase::Opening, 1.6f);
            }
            else
            {
                Teacher->SetActorHiddenInGame(true);
                bWaitForPlayerReturn = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildHideAndSeekGameMode>() != nullptr;
                for (AHumanityTrinityRebuildHider* NPC : Participants)
                    if (IsValid(NPC))
                        NPC->EndEmergencyHide();
                ChangePhase(ETeacherPatrolPhase::Recovering, 0);
            }
        }
    }
    else if (Phase == ETeacherPatrolPhase::Opening)
    {
        if (Now > PhaseEndsAt + 4.f)
        {
            Fail(TEXT("你挡住了教室门，老师发现了你。"));
            return;
        }
        if (Now >= PhaseEndsAt && Corridor->GetDoorOpenFraction(1) > .95f)
        {
            ChangePhase(ETeacherPatrolPhase::Inspecting, InspectionSeconds);
            CheckForDiscovery();
        }
    }
    else if (Phase == ETeacherPatrolPhase::Inspecting)
    {
        // Discovery is continuous: opening a prop door or switching a light on
        // during inspection is just as revealing as missing the warning.
        CheckForDiscovery();
        if (!HasFailed() && Now >= PhaseEndsAt)
        {
            ++CompletedPatrols;
            PlayTeacherSound(ClearVoice, 1.f);
            Corridor->SetDoorOpen(1, false);
            Teacher->SetWalking(true);
            ChangePhase(ETeacherPatrolPhase::Departing, 6.f);
            UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL] EVADED completed=%d"), CompletedPatrols);
        }
    }
    else if (Phase == ETeacherPatrolPhase::Recovering)
    {
        bool bNPCsReady = true;
        for (AHumanityTrinityRebuildHider* NPC : Participants)
            bNPCsReady &= !IsValid(NPC) || !NPC->IsEmergencyHiding();
        const auto* Player = UGameplayStatics::GetPlayerPawn(this, 0);
        const bool bPlayerReady = !bWaitForPlayerReturn || (Player && Player->GetActorLocation().Y > -1310.f);
        if (bNPCsReady && bPlayerReady)
            FinishPatrol();
    }
    else if (HasFailed() && Now >= PhaseEndsAt && !bSuppressFailureExit && !bExitRequested)
    {
        bExitRequested = true;
        UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL] FAILURE_EXIT"));
        UKismetSystemLibrary::QuitGame(this, UGameplayStatics::GetPlayerController(this, 0), EQuitPreference::Quit, false);
    }
}

float AHumanityTrinityRebuildTeacherPatrol::GetSecondsRemaining() const
{
    return FMath::Max(0.f, PhaseEndsAt - GetWorld()->GetTimeSeconds());
}
FString AHumanityTrinityRebuildTeacherPatrol::GetStatusLine() const
{
    switch (Phase)
    {
    case ETeacherPatrolPhase::Warning:
        return FString::Printf(TEXT("老师来了！%.0f秒内关灯、关闭屏幕，躲进舞台两侧暗房并关门。\n老师：这么晚了，里面还有人吗？我检查一下。"), FMath::CeilToFloat(GetSecondsRemaining()));
    case ETeacherPatrolPhase::Opening: return TEXT("老师正在开门——保持黑暗，留在关门的暗房里。走廊灯正照进来。 ");
    case ETeacherPatrolPhase::Inspecting: return FString::Printf(TEXT("老师正在门口检查 · %.0f秒\n不要开灯，不要打开暗房门。"), FMath::CeilToFloat(GetSecondsRemaining()));
    case ETeacherPatrolPhase::Departing: return TEXT("老师：嗯，没人。门关好了。\n脚步正在远去，请继续躲好。 ");
    case ETeacherPatrolPhase::Recovering: return TEXT("老师已经离开。按 E 打开暗房门，回到教室桌椅区；同伴返回后继续游戏。 ");
    case ETeacherPatrolPhase::Failed: return FString::Printf(TEXT("游戏失败：%s\n程序将在%.0f秒后退出。"), *FailureReason, FMath::CeilToFloat(GetSecondsRemaining()));
    default: return bEnabled ? TEXT("老师巡查已启用 · 留意门外的脚步和钥匙声") : TEXT("");
    }
}
