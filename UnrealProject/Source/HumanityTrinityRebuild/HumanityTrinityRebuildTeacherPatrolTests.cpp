#include "HumanityTrinityRebuildTeacherPatrolTests.h"
#include "HumanityTrinityRebuildTeacherPatrol.h"
#include "HumanityTrinityRebuildCorridor.h"
#include "HumanityTrinityRebuildDoorLayout.h"
#include "HumanityTrinityRebuildGameMode.h"
#include "HumanityTrinityRebuildHideAndSeekGameMode.h"
#include "HumanityTrinityRebuildPlayerCharacter.h"
#include "HumanityTrinityRebuildRoomInteraction.h"
#include "HumanityTrinityRebuildLightingController.h"
#include "HumanityTrinityRebuildHider.h"
#include "HumanityTrinityRebuildTeacher.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AHumanityTrinityRebuildTeacherPatrolTests::AHumanityTrinityRebuildTeacherPatrolTests()
{
    PrimaryActorTick.bCanEverTick = true;
}
void AHumanityTrinityRebuildTeacherPatrolTests::BeginPlay()
{
    Super::BeginPlay();
    FMath::RandInit(631075);
    FParse::Value(FCommandLine::Get(), TEXT("TeacherPatrolScenario="), Scenario);
    UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL_SELFTEST] BEGIN scenario=%s"), *Scenario);
}
void AHumanityTrinityRebuildTeacherPatrolTests::Check(const TCHAR* Name, bool bOK)
{
    bPassed &= bOK;
    UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL_SELFTEST] %s=%s"), Name, bOK ? TEXT("PASS") : TEXT("FAIL"));
}
void AHumanityTrinityRebuildTeacherPatrolTests::PlaceInRoom(int32 Index)
{
    const auto& Spec = HumanityPropDoors[Index];
    const FVector Centre = Spec.Hinge + FRotator(0, Spec.Yaw, 0).RotateVector(FVector(Spec.Width / 2, 0, 0));
    Player->SetActorLocation(Centre - Spec.StageNormal * 110 + FVector(0, 0, 96), false, nullptr, ETeleportType::TeleportPhysics);
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->GetController()->SetControlRotation(FRotator(-80, 0, 0));
}
void AHumanityTrinityRebuildTeacherPatrolTests::Capture(const TCHAR* Name)
{
    const auto* Hide = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildHideAndSeekGameMode>();
    const TCHAR* Mode = !Hide ? TEXT("walk") : (Hide->IsPlayerHiding() ? TEXT("hide") : (Hide->IsPracticeMode() ? TEXT("practice") : TEXT("seek")));
    if (FParse::Param(FCommandLine::Get(), TEXT("TeacherPatrolCapture")))
        FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"),
            FString::Printf(TEXT("TeacherPatrol_%s_%s_%s.png"), Mode, *Scenario, Name)), false, false);
}
void AHumanityTrinityRebuildTeacherPatrolTests::Finish()
{
    if (bFinished) return;
    bFinished = true;
    UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL_SELFTEST] %s scenario=%s npc_travel=%.1f"),
        bPassed ? TEXT("PASS") : TEXT("FAIL"), *Scenario, NPCDistance);
    FPlatformMisc::RequestExit(false);
}
void AHumanityTrinityRebuildTeacherPatrolTests::Tick(float Delta)
{
    Super::Tick(Delta);
    if (bFinished) return;
    Elapsed += Delta;
    PhaseElapsed += Delta;
    if (Elapsed > 125) { Check(TEXT("bounded_completion"), false); Finish(); return; }
    auto* Game = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildGameMode>();
    auto* Hide = GetWorld()->GetAuthGameMode<AHumanityTrinityRebuildHideAndSeekGameMode>();
    if (Step == 0)
    {
        if (Elapsed < 3) return;
        if (Hide && Hide->IsPlayerHiding() && !Hide->IsRoundRunning()) Hide->ReadyToHide();
        if (Hide && !Hide->IsRoundRunning()) return;
        Patrol = AHumanityTrinityRebuildTeacherPatrol::Find(GetWorld());
        Player = Cast<AHumanityTrinityRebuildPlayerCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
        for (TActorIterator<AHumanityTrinityRebuildRoomInteraction> It(GetWorld()); It; ++It) Room = *It;
        for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It) Lighting = *It;
        for (TActorIterator<AHumanityTrinityRebuildCorridor> It(GetWorld()); It; ++It) Corridor = *It;
        for (TActorIterator<AHumanityTrinityRebuildHider> It(GetWorld()); It; ++It) NPC = *It;
        Check(TEXT("actors_ready"), Patrol && Player && Room && Lighting && Corridor);
        if (!bPassed) { Finish(); return; }
        Check(TEXT("all_five_teacher_sounds_loaded"), Patrol->HasAllAudio());
        Check(TEXT("four_corridor_fixtures"), Corridor->GetLitFixtureCount() == 4);
        int32 TeacherParts = 0;
        for (TActorIterator<AHumanityTrinityRebuildTeacher> It(GetWorld()); It; ++It) TeacherParts = It->GetLoadedPartCount();
        Check(TEXT("teacher_all_fifteen_parts_loaded"), TeacherParts == 15);
        Patrol->bSuppressFailureExit = Scenario != TEXT("exit");
        Patrol->SetEnabled(false);
        Check(TEXT("opt_out_rejects_trigger"), !Patrol->StartPatrol() && !Patrol->IsActive());
        for (int32 I = 0; I < 3; ++I)
        {
            Check(*FString::Printf(TEXT("exterior_door_%d_default_closed"), I), Corridor->GetDoorOpenFraction(I) < .001f);
            const FVector Centre = Corridor->GetDoorwayLocation(I) + FVector(0, 0, 110);
            FHitResult Hit;
            FCollisionQueryParams Q(SCENE_QUERY_STAT(PatrolDoorClosed), true, Player);
            Check(*FString::Printf(TEXT("exterior_door_%d_blocks_visibility"), I),
                GetWorld()->LineTraceSingleByChannel(Hit, Centre + FVector(100,0,0), Centre - FVector(100,0,0), ECC_Visibility, Q)
                && Hit.GetActor() == Corridor);
        }
        Player->SetActorLocation(FVector(0,-260,96));
        Camera = GetWorld()->SpawnActor<ACameraActor>(FVector(-370,-1080,158), FRotator(-4,-158,0));
        auto& CameraSettings = Camera->GetCameraComponent()->PostProcessSettings;
        CameraSettings.bOverride_AutoExposureMethod = true;
        CameraSettings.AutoExposureMethod = AEM_Manual;
        CameraSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
        CameraSettings.AutoExposureApplyPhysicalCameraExposure = false;
        CameraSettings.bOverride_AutoExposureBias = true;
        CameraSettings.AutoExposureBias = -4.7f;
        Camera->GetCameraComponent()->PostProcessBlendWeight = 1.f;
        GetWorld()->GetFirstPlayerController()->SetViewTarget(Camera);
        Capture(TEXT("Closed"));
        Step = 1; PhaseElapsed = 0;
        return;
    }
    if (Step == 1)
    {
        if (PhaseElapsed < 2) return;
        Check(TEXT("disabled_stays_dormant"), !Patrol->IsActive());
        Patrol->FirstDelayMin = Patrol->FirstDelayMax = Scenario == TEXT("auto") ? .25f : 200.f;
        Patrol->RepeatDelayMin = Patrol->RepeatDelayMax = 200;
        Patrol->WarningSeconds = NPC ? 26 : 6;
        Patrol->InspectionSeconds = 5;
        Patrol->SetEnabled(true);
        if (Scenario != TEXT("auto")) Check(TEXT("explicit_opt_in_starts"), Patrol->StartPatrol());
        Lighting->SetMasterLights(false);
        Room->SetScreenOn(false);
        if (Scenario != TEXT("auto"))
        {
            Lighting->SetMasterLights(true);
            Room->SetScreenOn(true);
            bool bBound = false;
            if (Player->InputComponent)
                for (int32 I = 0; I < Player->InputComponent->GetNumActionBindings(); ++I)
                {
                    auto& Action = Player->InputComponent->GetActionBinding(I);
                    if (Action.GetActionName() == TEXT("ToggleMasterLights") && Action.KeyEvent == IE_Pressed)
                    {
                        bBound = true;
                        Action.ActionDelegate.Execute(EKeys::L);
                        Action.ActionDelegate.Execute(EKeys::L); // A repeated emergency press must stay dark.
                    }
                }
            Check(TEXT("emergency_L_binding_extinguishes_lights_and_display"), bBound && !Lighting->AreMainLightsOn() && !Room->IsScreenOn());
        }
        if (NPC) PreviousNPC = NPC->GetActorLocation();
        InitialRoundTime = Hide ? Hide->GetSecondsRemaining() : 0;
        PlaceInRoom(0);
        Check(TEXT("right_closed_dark_room_safe"), Patrol->IsActorSafelyHidden(Player));
        PlaceInRoom(1);
        Check(TEXT("left_closed_dark_room_safe"), Patrol->IsActorSafelyHidden(Player));
        Player->SetActorLocation(FVector(0,-1650,138));
        Check(TEXT("stage_is_not_a_hiding_room"), !Patrol->IsActorSafelyHidden(Player));
        // Keep player opposite the NPC's selected shelter to test two independent doors.
        PlaceInRoom(NPC ? 1 - NPC->GetEmergencyDoorIndex() : 0);
        if (Scenario == TEXT("exposed") || Scenario == TEXT("exit"))
        {
            Player->SetActorLocation(FVector(-300,-1100,96));
            Player->GetController()->SetControlRotation(FRotator(-4, -162, 0));
        }
        if (Scenario == TEXT("lights")) Lighting->SetMasterLights(true);
        if (Scenario == TEXT("screen")) Room->SetScreenOn(true);
        Step = 2; PhaseElapsed = 0;
        return;
    }
    if (NPC)
    {
        NPCDistance += FVector::Distance(PreviousNPC, NPC->GetActorLocation());
        PreviousNPC = NPC->GetActorLocation();
    }
    if (Step == 2)
    {
        if (Hide && Patrol->GetPhase() == ETeacherPatrolPhase::Recovering && Player->GetActorLocation().Y < -1310)
            Player->SetActorLocation(FVector(0, -1260, 96), false, nullptr, ETeleportType::TeleportPhysics);
        if (PhaseElapsed > 1 && PhaseElapsed < 1 + Delta * 2) Capture(TEXT("Warning"));
        if (Scenario == TEXT("auto") && PhaseElapsed > 1 && PhaseElapsed < 1 + Delta * 2)
            Check(TEXT("enabled_timer_triggers_automatically"), Patrol->IsActive());
        if (Hide && PhaseElapsed > 1 && PhaseElapsed < 1 + Delta * 2)
            Check(TEXT("round_timer_paused"), FMath::IsNearlyEqual(Hide->GetSecondsRemaining(), InitialRoundTime, .12f));
        if (Patrol->GetPhase() == ETeacherPatrolPhase::Inspecting && !bCapturedInspection)
        {
            bCapturedInspection = true;
            Check(TEXT("inspection_door_open"), Corridor->GetDoorOpenFraction(1) > .9f);
            if (NPC) Check(TEXT("npc_physically_reached_shelter"), NPCDistance > 200 && Patrol->IsActorSafelyHidden(NPC));
            Capture(TEXT("Inspection"));
            if (Scenario == TEXT("late")) Room->SetPropDoorOpen(0, true);
        }
        const bool bExpectFailure = Scenario == TEXT("exposed") || Scenario == TEXT("lights") || Scenario == TEXT("screen") || Scenario == TEXT("late") || Scenario == TEXT("exit");
        if (Patrol->HasFailed())
        {
            Check(TEXT("correct_failure_result"), bExpectFailure);
            UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL_SELFTEST] failure_reason=%s"), *Patrol->GetFailureReason());
            Capture(TEXT("Failed"));
            Step = Scenario == TEXT("exit") ? 5 : 3; PhaseElapsed = 0;
            if (Step == 5)
                UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL_SELFTEST] exit_pending=%s"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
        }
        else if (Patrol->GetCompletedPatrolCount() > 0 && !Patrol->IsActive())
        {
            Check(TEXT("safe_inspection_completes"), !bExpectFailure && bCapturedInspection);
            Check(TEXT("door_closed_after_departure"), Corridor->GetDoorOpenFraction(1) < .001f);
            if (NPC) Check(TEXT("npc_resumes_after_return"), !NPC->IsEmergencyHiding());
            if (Hide) Check(TEXT("round_remains_running"), Hide->IsRoundRunning() && !Hide->IsRoundFinished());
            Capture(TEXT("Safe"));
            Step = 3; PhaseElapsed = 0;
        }
    }
    else if (Step == 3 && PhaseElapsed > 1)
    {
        if (Scenario == TEXT("exposed"))
        {
            Check(TEXT("player_adapts_to_visible_corridor"), Player->GetCurrentExposure() < -4.f);
            UE_LOG(LogTemp, Display, TEXT("[TEACHER_PATROL_SELFTEST] player_corridor_exposure=%.3f"), Player->GetCurrentExposure());
        }
        if (Hide && !Patrol->HasFailed()) Check(TEXT("timer_resumes"), Hide->GetSecondsRemaining() < InitialRoundTime - .3f);
        Patrol->CancelPatrol();
        Patrol->SetEnabled(false);
        Check(TEXT("cancel_clears_failure"), !Patrol->HasFailed());
        Step = 4; PhaseElapsed = 0;
    }
    else if (Step == 4 && PhaseElapsed > .25f && !Patrol->IsActive())
    {
        Check(TEXT("cancel_returns_to_dormant"), !Patrol->HasFailed());
        Finish();
    }
}
