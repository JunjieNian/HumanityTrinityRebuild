#include "HumanityTrinityRebuildGameMode.h"

#include "HumanityTrinityRebuildEnvironmentActor.h"
#include "HumanityTrinityRebuildHUD.h"
#include "HumanityTrinityRebuildLightSwitch.h"
#include "HumanityTrinityRebuildLightingController.h"
#include "HumanityTrinityRebuildPlayerCharacter.h"
#include "HumanityTrinityRebuildRoomInteraction.h"
#include "HumanityTrinityRebuildDoorLayout.h"
#include "HumanityTrinityRebuildTeachingLayout.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

AHumanityTrinityRebuildGameMode::AHumanityTrinityRebuildGameMode()
{
    DefaultPawnClass = AHumanityTrinityRebuildPlayerCharacter::StaticClass();
    HUDClass = AHumanityTrinityRebuildHUD::StaticClass();
}

void AHumanityTrinityRebuildGameMode::BeginPlay()
{
    Super::BeginPlay();

    bool bHasEnvironment = false;
    for (TActorIterator<AHumanityTrinityRebuildEnvironmentActor> It(GetWorld()); It; ++It)
    {
        bHasEnvironment = true;
        break;
    }
    if (!bHasEnvironment)
    {
        GetWorld()->SpawnActor<AHumanityTrinityRebuildEnvironmentActor>(AHumanityTrinityRebuildEnvironmentActor::StaticClass(), FTransform::Identity);
    }

    bool bHasLighting = false;
    for (TActorIterator<AHumanityTrinityRebuildLightingController> It(GetWorld()); It; ++It)
    {
        bHasLighting = true;
        SelfTestLighting = *It;
        break;
    }
    if (!bHasLighting)
    {
        SelfTestLighting = GetWorld()->SpawnActor<AHumanityTrinityRebuildLightingController>(
            AHumanityTrinityRebuildLightingController::StaticClass(),
            FTransform::Identity);
    }

    bool bHasSwitch = false;
    for (TActorIterator<AHumanityTrinityRebuildLightSwitch> It(GetWorld()); It; ++It)
    {
        bHasSwitch = true;
        break;
    }
    if (!bHasSwitch)
    {
        // Front-door switch on the left wall. The wall's classroom-facing
        // surface is X=-600 cm; a 2.5 cm plate centered at -598.75 sits flush.
        const FVector SwitchLocation(-598.75f, -95.0f, 125.0f);
        const FRotator SwitchRotation(0.0f, 0.0f, 0.0f);
        GetWorld()->SpawnActor<AHumanityTrinityRebuildLightSwitch>(
            AHumanityTrinityRebuildLightSwitch::StaticClass(),
            SwitchLocation,
            SwitchRotation);
    }

    for (TActorIterator<AHumanityTrinityRebuildRoomInteraction> It(GetWorld()); It; ++It)
    {
        SelfTestRoom = *It;
        break;
    }
    if (!SelfTestRoom)
    {
        SelfTestRoom = GetWorld()->SpawnActor<AHumanityTrinityRebuildRoomInteraction>(
            AHumanityTrinityRebuildRoomInteraction::StaticClass(), FTransform::Identity);
    }

    if (FParse::Param(FCommandLine::Get(), TEXT("HumanityTrinityRebuildSelfTest")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AHumanityTrinityRebuildGameMode::StartSelfTest);
    }
}

void AHumanityTrinityRebuildGameMode::StartSelfTest()
{
#if WITH_EDITOR
    // First-load captures must not show temporary checkerboard shader fallbacks.
    if (GShaderCompilingManager)
    {
        GShaderCompilingManager->FinishAllCompilation();
    }
#endif
    bCaptureSelfTestScreenshots = FParse::Param(FCommandLine::Get(), TEXT("HumanityTrinityRebuildCapture"));
    SelfTestPlayer = Cast<AHumanityTrinityRebuildPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!SelfTestLighting || !SelfTestPlayer || !SelfTestRoom)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] FAIL missing actors lighting=%s player=%s"),
            SelfTestLighting ? TEXT("yes") : TEXT("no"),
            SelfTestPlayer ? TEXT("yes") : TEXT("no"));
        FPlatformMisc::RequestExit(false);
        return;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] START main=%d residual=%d exposure=%.3f"),
        SelfTestLighting->GetActiveMainLightCount(),
        SelfTestLighting->GetActiveResidualLightCount(),
        SelfTestPlayer->GetCurrentExposure());

    bSelfTestPassed &= SelfTestRoom->HasCurtainMesh() && !SelfTestRoom->IsScreenOn();
    if (FParse::Param(FCommandLine::Get(), TEXT("HumanityTrinityRebuildDoorTest")))
    {
        SelfTestPreparePropDoor();
        return;
    }
    const bool bPanelsInitiallyOn = SelfTestLighting->GetEmittingPanelCount() == 20;
    for (const EHumanityTrinityRebuildLightZone Zone : {EHumanityTrinityRebuildLightZone::Front,
        EHumanityTrinityRebuildLightZone::Middle, EHumanityTrinityRebuildLightZone::Rear, EHumanityTrinityRebuildLightZone::Stage})
    {
        SelfTestLighting->SetZoneEnabled(Zone, false);
    }
    const bool bZonesDark = !SelfTestLighting->AreMainLightsOn() && SelfTestLighting->GetEmittingPanelCount() == 0
        && SelfTestLighting->GetActiveCeilingBounceCount() == 0;
    SelfTestLighting->ToggleMaster();
    const bool bMasterRecovered = SelfTestLighting->GetActiveMainLightCount() == 9 && SelfTestLighting->GetEmittingPanelCount() == 20;
    bSelfTestPassed &= bPanelsInitiallyOn && bZonesDark && bMasterRecovered;
    UE_LOG(LogTemp, Display, TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] PANELS on=%s zones_dark=%s recovered=%s"),
        bPanelsInitiallyOn ? TEXT("yes") : TEXT("no"), bZonesDark ? TEXT("yes") : TEXT("no"), bMasterRecovered ? TEXT("yes") : TEXT("no"));

    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestCaptureLightsOn, 2.0f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestCaptureLightsOn()
{
    CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_00_LightsOn.png"));

    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestSwitchOff, 0.8f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestSwitchOff()
{
    bSelfTestPassed &= SelfTestLighting->GetActiveMainLightCount() == 9;
    bSelfTestPassed &= SelfTestLighting->GetActiveResidualLightCount() == 0;

    SelfTestDarkExposureStart = SelfTestPlayer->GetCurrentExposure();
    SelfTestLighting->SetMasterLights(false);

    const int32 MainAfterOff = SelfTestLighting->GetActiveMainLightCount();
    const int32 ResidualAfterOff = SelfTestLighting->GetActiveResidualLightCount();
    bSelfTestPassed &= MainAfterOff == 0;
    bSelfTestPassed &= ResidualAfterOff == 2;
    bSelfTestPassed &= SelfTestLighting->GetEmittingPanelCount() == 0;
    bSelfTestPassed &= SelfTestLighting->GetActiveCeilingBounceCount() == 0;

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] OFF main=%d residual=%d exposure_start=%.3f"),
        MainAfterOff,
        ResidualAfterOff,
        SelfTestDarkExposureStart);

    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestCaptureJustOff, 1.5f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestCaptureJustOff()
{
    // Give the renderer one short settling interval so the screenshot does not
    // contain the previous frame's Lumen/TAA history. Exposure is still close
    // to the light-adapted value, so this remains the immediate-dark state.
    CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_01_JustOff.png"));

    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestObserveDark, 6.5f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestObserveDark()
{
    SelfTestDarkExposureObserved = SelfTestPlayer->GetCurrentExposure();
    bSelfTestPassed &= SelfTestDarkExposureObserved > SelfTestDarkExposureStart + 0.25f;

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] DARK_ADAPTED exposure=%.3f"),
        SelfTestDarkExposureObserved);

    CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_02_DarkAdapted.png"));

    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestTurnOn, 0.6f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestTurnOn()
{
    SelfTestLighting->SetMasterLights(true);

    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestFinish, 2.0f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestFinish()
{
    const float BrightExposure = SelfTestPlayer->GetCurrentExposure();
    const int32 MainAfterOn = SelfTestLighting->GetActiveMainLightCount();
    const int32 ResidualAfterOn = SelfTestLighting->GetActiveResidualLightCount();

    bSelfTestPassed &= MainAfterOn == 9;
    bSelfTestPassed &= ResidualAfterOn == 0;
    const float LightAdaptedTarget = SelfTestPlayer->GetLightAdaptedExposure();
    const float DarkDistance = FMath::Abs(SelfTestDarkExposureObserved - LightAdaptedTarget);
    const float BrightDistance = FMath::Abs(BrightExposure - LightAdaptedTarget);
    bSelfTestPassed &= BrightDistance < DarkDistance * 0.35f;

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] LIGHT_RECOVERY main=%d residual=%d exposure_after_bright=%.3f"),
        MainAfterOn,
        ResidualAfterOn,
        BrightExposure);

    CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_03_LightsRestored.png"));

    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestPrepareSwitchView, 0.8f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestPrepareSwitchView()
{
    // Put the real player/camera where a visitor would stand to use the switch.
    // This lets the automated test exercise the same visibility trace as E.
    // Use a close oblique view so the screenshot proves that the paddle
    // protrudes from the wall instead of being embedded or reversed.
    SelfTestPlayer->SetActorLocation(FVector(-510.0f, -45.0f, 96.0f), false, nullptr, ETeleportType::TeleportPhysics);
    if (AController* Controller = SelfTestPlayer->GetController())
    {
        Controller->SetControlRotation(FRotator(-20.5f, -150.6f, 0.0f));
    }

    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestValidateSwitch, 0.8f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestValidateSwitch()
{
    CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_04_LightSwitch.png"));

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    FVector ViewLocation;
    FRotator ViewRotation;
    PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HumanityTrinityRebuildSwitchSelfTest), false, SelfTestPlayer);
    const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * 240.0f;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
    AHumanityTrinityRebuildLightSwitch* HitSwitch = bHit ? Cast<AHumanityTrinityRebuildLightSwitch>(Hit.GetActor()) : nullptr;
    const bool bTraceFoundSwitch = HitSwitch != nullptr;
    bSelfTestPassed &= bTraceFoundSwitch;

    bool bToggledOff = false;
    bool bToggledBackOn = false;
    if (HitSwitch)
    {
        HitSwitch->Interact(SelfTestPlayer);
        bToggledOff = !SelfTestLighting->AreMainLightsOn();
        HitSwitch->Interact(SelfTestPlayer);
        bToggledBackOn = SelfTestLighting->AreMainLightsOn();
        bSelfTestPassed &= bToggledOff && bToggledBackOn;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] SWITCH trace=%s actor=%s distance=%.1f toggled_off=%s restored=%s"),
        bTraceFoundSwitch ? TEXT("PASS") : TEXT("FAIL"),
        Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("none"),
        bHit ? Hit.Distance : -1.0f,
        bToggledOff ? TEXT("yes") : TEXT("no"),
        bToggledBackOn ? TEXT("yes") : TEXT("no"));

    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestPrepareStageView, 0.8f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestPrepareStageView()
{
    SelfTestRoom->SetCurtainsOpen(true);
    SelfTestPlayer->SetActorLocation(FVector(-480.0f, -1250.0f, 96.0f), false, nullptr, ETeleportType::TeleportPhysics);
    if (AController* Controller = SelfTestPlayer->GetController())
    {
        Controller->SetControlRotation(FRotator(0.0f, -38.0f, 0.0f));
    }
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestValidateCurtainsOpen, 3.0f, false);
}

bool AHumanityTrinityRebuildGameMode::SelfTestCurtainBlocksPassage() const
{
    // Sweep a visitor-sized capsule across the centre of the stage opening.
    // Its base clears both steps, isolating the moving drape's collision.
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HumanityTrinityRebuildCurtainPassage), false, SelfTestPlayer);
    const bool bHit = GetWorld()->SweepSingleByChannel(Hit, FVector(40.0f, -1390.0f, 171.0f),
        FVector(40.0f, -1490.0f, 171.0f), FQuat::Identity, ECC_Pawn,
        FCollisionShape::MakeCapsule(34.0f, 92.0f), Params);
    return bHit && Hit.GetActor() == SelfTestRoom;
}

void AHumanityTrinityRebuildGameMode::SelfTestValidateCurtainsOpen()
{
    const bool bClear = !SelfTestCurtainBlocksPassage();
    const bool bFullyOpen = SelfTestRoom->GetCurtainOpenFraction() > 0.99f;
    bSelfTestPassed &= bClear && bFullyOpen && SelfTestRoom->HasCurtainMesh();
    UE_LOG(LogTemp, Display, TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] CURTAIN_OPEN fraction=%.3f passage_clear=%s mesh=%s"),
        SelfTestRoom->GetCurtainOpenFraction(), bClear ? TEXT("yes") : TEXT("no"),
        SelfTestRoom->HasCurtainMesh() ? TEXT("yes") : TEXT("no"));
    CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_05_StageOpen.png"));

    // Exercise the E path by hitting the real left curtain hem at arm's reach.
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HumanityTrinityRebuildCurtainUse), false, SelfTestPlayer);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, FVector(-515.0f, -1320.0f, 170.0f),
        FVector(-515.0f, -1460.0f, 170.0f), ECC_Visibility, Params);
    const bool bCurtainHit = bHit && Hit.GetActor() == SelfTestRoom && SelfTestRoom->IsCurtainComponent(Hit.GetComponent());
    bSelfTestPassed &= bCurtainHit;
    if (bCurtainHit)
    {
        SelfTestRoom->Interact(Hit.GetComponent());
    }
    else
    {
        SelfTestRoom->SetCurtainsOpen(false);
    }
    UE_LOG(LogTemp, Display, TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] CURTAIN_USE trace=%s"), bCurtainHit ? TEXT("PASS") : TEXT("FAIL"));
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestValidateCurtainsClosed, 3.2f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestValidateCurtainsClosed()
{
    const bool bBlocked = SelfTestCurtainBlocksPassage();
    const bool bFullyClosed = SelfTestRoom->GetCurtainOpenFraction() < 0.01f;
    bSelfTestPassed &= bBlocked && bFullyClosed;
    UE_LOG(LogTemp, Display, TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] CURTAIN_CLOSED fraction=%.3f passage_blocked=%s"),
        SelfTestRoom->GetCurtainOpenFraction(), bBlocked ? TEXT("yes") : TEXT("no"));
    CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_06_StageClosed.png"));
    SelfTestRoom->ToggleCurtains();
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestValidateCurtainsReopened, 3.2f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestValidateCurtainsReopened()
{
    const bool bOpenAgain = SelfTestRoom->GetCurtainOpenFraction() > 0.99f && !SelfTestCurtainBlocksPassage();
    bSelfTestPassed &= bOpenAgain;
    UE_LOG(LogTemp, Display, TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] CURTAIN_REOPENED %s"), bOpenAgain ? TEXT("PASS") : TEXT("FAIL"));
    SelfTestPrepareBoardView();
}

bool AHumanityTrinityRebuildGameMode::SelfTestScreenOcclusion(bool bExpectScreen) const
{
    // Test the centre and eight near-edge points, not just a target/state flag.
    FCollisionQueryParams Params(SCENE_QUERY_STAT(TeachingBoardOcclusion), true, SelfTestPlayer);
    for (float X : {-0.45f, 0.0f, 0.45f})
    for (float Z : {-0.45f, 0.0f, 0.45f})
    {
        const FVector End = HumanityTeaching::Screen + FVector(X*HumanityTeaching::ScreenSize.X, 0, Z*HumanityTeaching::ScreenSize.Z);
        FHitResult Hit;
        if (!GetWorld()->LineTraceSingleByChannel(Hit, End-FVector(0,180,0), End, ECC_Visibility, Params)) return false;
        if (Hit.GetActor() != SelfTestRoom) return false;
        if (bExpectScreen ? !SelfTestRoom->IsScreenComponent(Hit.GetComponent())
                          : !SelfTestRoom->IsMovingBoardComponent(Hit.GetComponent())) return false;
    }
    return true;
}

void AHumanityTrinityRebuildGameMode::SelfTestPrepareBoardView()
{
    SelfTestPlayer->SetActorLocation(FVector(0,-330,96),false,nullptr,ETeleportType::TeleportPhysics);
    SelfTestPlayer->GetController()->SetControlRotation(FRotator(4,90,0));
    const bool bClosed = SelfTestRoom->GetBoardLocation().Equals(HumanityTeaching::Closed,.1f)
        && SelfTestScreenOcclusion(false) && !SelfTestRoom->IsScreenIlluminating();
    bSelfTestPassed &= bClosed;
    UE_LOG(LogTemp,Display,TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] BOARD_INITIAL_CLOSED %s"),bClosed?TEXT("PASS"):TEXT("FAIL"));
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,[this]() {
        CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_14_BoardClosed.png"));
        FTimerHandle Next;
        GetWorldTimerManager().SetTimer(Next,this,&AHumanityTrinityRebuildGameMode::SelfTestPrepareScreenControl,.8f,false);
    },1.2f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestPrepareScreenControl()
{
    SelfTestPlayer->SetActorLocation(FVector(-358.0f, -230.0f, 96.0f), false, nullptr, ETeleportType::TeleportPhysics);
    if (AController* Controller = SelfTestPlayer->GetController())
    {
        Controller->SetControlRotation(FRotator(-19.4f, 90.0f, 0.0f));
    }
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestValidateScreenControl, 1.0f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestValidateScreenControl()
{
    FVector ViewLocation;
    FRotator ViewRotation;
    UGameplayStatics::GetPlayerController(this, 0)->GetPlayerViewPoint(ViewLocation, ViewRotation);
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HumanityTrinityRebuildScreenUse), false, SelfTestPlayer);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation,
        ViewLocation + ViewRotation.Vector() * 240.0f, ECC_Visibility, Params);
    const bool bControlHit = bHit && Hit.GetActor() == SelfTestRoom && SelfTestRoom->IsScreenComponent(Hit.GetComponent());
    if (bControlHit)
    {
        SelfTestRoom->Interact(Hit.GetComponent());
    }
    const bool bScreenOn = SelfTestRoom->IsScreenOn() && !SelfTestRoom->IsScreenIlluminating();
    bSelfTestPassed &= bControlHit && bScreenOn;
    UE_LOG(LogTemp, Display, TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] SCREEN_USE trace=%s powered=%s local_light=%s"),
        bControlHit ? TEXT("PASS") : TEXT("FAIL"), SelfTestRoom->IsScreenOn() ? TEXT("on") : TEXT("off"),
        SelfTestRoom->IsScreenIlluminating() ? TEXT("on") : TEXT("off"));

    SelfTestPlayer->SetActorLocation(FVector(0,-330,96),false,nullptr,ETeleportType::TeleportPhysics);
    SelfTestPlayer->GetController()->SetControlRotation(FRotator(4,90,0));
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestReverseBoard, .65f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestReverseBoard()
{
    const float Before = SelfTestRoom->GetBoardOpenFraction();
    const FVector Position = SelfTestRoom->GetBoardLocation();
    SelfTestRoom->ToggleScreen();
    const bool bSmooth = Before > 0 && Before < 1 && Position.Equals(SelfTestRoom->GetBoardLocation(),.001f)
        && !SelfTestRoom->IsScreenIlluminating();
    bSelfTestPassed &= bSmooth;
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,[this,Before,bSmooth]() {
        const bool bReversed = bSmooth && SelfTestRoom->GetBoardOpenFraction() < Before;
        bSelfTestPassed &= bReversed;
        UE_LOG(LogTemp,Display,TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] BOARD_MID_TRAVEL_REVERSE %s"),bReversed?TEXT("PASS"):TEXT("FAIL"));
        SelfTestRoom->ToggleScreen();
        FTimerHandle Next;
        GetWorldTimerManager().SetTimer(Next,this,&AHumanityTrinityRebuildGameMode::SelfTestValidateBoardOpen,
            HumanityTeaching::TravelSeconds+1.0f,false);
    },.25f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestValidateBoardOpen()
{
    // With yaw=90, camera-right is -X: the fixed screen must be on that side,
    // while the moving board travels +X to overlap the left panel.
    const FVector ViewRight = FRotationMatrix(FRotator(0,90,0)).GetUnitAxis(EAxis::Y);
    const bool bRight = FVector::DotProduct(HumanityTeaching::Screen,ViewRight)>0
        && FVector::DotProduct(SelfTestRoom->GetBoardLocation()-HumanityTeaching::Closed,ViewRight)<0;
    const bool bOpen = SelfTestRoom->GetBoardLocation().Equals(HumanityTeaching::Open,.1f)
        && SelfTestRoom->IsScreenIlluminating() && SelfTestScreenOcclusion(true) && bRight;
    bSelfTestPassed &= bOpen;
    UE_LOG(LogTemp,Display,TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] BOARD_REVEAL_RIGHT_SCREEN %s"),bOpen?TEXT("PASS"):TEXT("FAIL"));
    CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_15_BoardOpen.png"));
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,[this]() {
        SelfTestPlayer->SetActorLocation(FVector(-485,-1280,96),false,nullptr,ETeleportType::TeleportPhysics);
        SelfTestPlayer->GetController()->SetControlRotation(FRotator(-1,65,0));
        FTimerHandle Next;
        GetWorldTimerManager().SetTimer(Next,this,&AHumanityTrinityRebuildGameMode::SelfTestCaptureTeachingView,1.8f,false);
    },.8f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestCaptureTeachingView()
{
    CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_07_TeachingView.png"));
    // Separate the capture from the power transition by one timer interval.
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestValidateScreenOff, 0.8f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestValidateScreenOff()
{
    SelfTestRoom->ToggleScreen();
    const bool bScreenOff = !SelfTestRoom->IsScreenOn() && !SelfTestRoom->IsScreenIlluminating();
    bSelfTestPassed &= bScreenOff;
    UE_LOG(LogTemp, Display, TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] SCREEN_OFF %s"), bScreenOff ? TEXT("PASS") : TEXT("FAIL"));
    SelfTestPlayer->SetActorLocation(FVector(0,-330,96),false,nullptr,ETeleportType::TeleportPhysics);
    SelfTestPlayer->GetController()->SetControlRotation(FRotator(4,90,0));
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,[this]() {
        const bool bClosed = SelfTestRoom->GetBoardLocation().Equals(HumanityTeaching::Closed,.1f)
            && SelfTestScreenOcclusion(false) && !SelfTestRoom->IsScreenIlluminating();
        bSelfTestPassed &= bClosed;
        UE_LOG(LogTemp,Display,TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] BOARD_RETURN_AND_OCCLUDE %s"),bClosed?TEXT("PASS"):TEXT("FAIL"));
        CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_16_BoardReturned.png"));
        FTimerHandle Next;
        GetWorldTimerManager().SetTimer(Next,this,&AHumanityTrinityRebuildGameMode::SelfTestPrepareCeilingView,.8f,false);
    },HumanityTeaching::TravelSeconds+.6f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestPrepareCeilingView()
{
    SelfTestPlayer->SetActorLocation(FVector(-80,-465,169), false, nullptr, ETeleportType::TeleportPhysics);
    SelfTestPlayer->GetController()->SetControlRotation(FRotator(48,-80,0));
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,this,&AHumanityTrinityRebuildGameMode::SelfTestCaptureCeilingView,2.0f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestCaptureCeilingView()
{
    FVector View; FRotator Rotation;
    UGameplayStatics::GetPlayerController(this,0)->GetPlayerViewPoint(View,Rotation);
    FHitResult Support;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(TableSupport),false,SelfTestPlayer);
    const FVector Position = SelfTestPlayer->GetActorLocation();
    const bool bSupport = GetWorld()->LineTraceSingleByChannel(Support,Position,Position-FVector(0,0,120),ECC_Visibility,Query);
    const float FootGap = Position.Z-SelfTestPlayer->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-Support.ImpactPoint.Z;
    const bool bOnTable = bSupport && FMath::IsNearlyEqual(Support.ImpactPoint.Z,76.0f,1.0f) && FootGap>=0 && FootGap<3;
    bSelfTestPassed &= bOnTable && View.Z < 250.0f && SelfTestLighting->GetActiveCeilingBounceCount()==9;
    UE_LOG(LogTemp,Display,TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] TABLE_VIEW on_table=%s eye_z=%.1f ceiling_clearance=%.1f bounce=%d"),
        bOnTable?TEXT("yes"):TEXT("no"),View.Z,340.0f-View.Z,SelfTestLighting->GetActiveCeilingBounceCount());
    CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_08_TableCeiling.png"));
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,[this]() {
        SelfTestPlayer->SetActorLocation(FVector(-162,-390,96),false,nullptr,ETeleportType::TeleportPhysics);
        SelfTestPlayer->GetController()->SetControlRotation(FRotator(-32,-90,0));
        FTimerHandle Next;
        GetWorldTimerManager().SetTimer(Next,this,&AHumanityTrinityRebuildGameMode::SelfTestCaptureChairView,1.5f,false);
    },0.7f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestCaptureChairView()
{
    CaptureSelfTestScreenshot(TEXT("HumanityTrinityRebuild_09_ChairClearance.png"));
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,this,&AHumanityTrinityRebuildGameMode::SelfTestPreparePropDoor,0.7f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestPreparePropDoor()
{
    const auto& Spec = HumanityPropDoors[SelfTestDoorIndex];
    const FVector Centre = Spec.Hinge + FRotator(0,Spec.Yaw,0).RotateVector(FVector(Spec.Width/2,0,0));
    SelfTestPlayer->SetActorLocation(Centre+Spec.StageNormal*200+FVector(0,0,94),false,nullptr,ETeleportType::TeleportPhysics);
    const FVector Aim = Centre+FVector(0,0,Spec.Height*.55f);
    SelfTestPlayer->GetController()->SetControlRotation((Aim-(SelfTestPlayer->GetActorLocation()+FVector(0,0,66))).Rotation());
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,this,&AHumanityTrinityRebuildGameMode::SelfTestOpenPropDoor,1.5f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestOpenPropDoor()
{
    CaptureSelfTestScreenshot(FString::Printf(TEXT("HumanityTrinityRebuild_10_PropDoor%d_Closed.png"),SelfTestDoorIndex));
    FVector View; FRotator Rotation;
    UGameplayStatics::GetPlayerController(this,0)->GetPlayerViewPoint(View,Rotation);
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(PropDoorUse),false,SelfTestPlayer);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit,View,View+Rotation.Vector()*240,ECC_Visibility,Query);
    const bool bDoor = bHit && Hit.GetActor()==SelfTestRoom && SelfTestRoom->GetPropDoorIndex(Hit.GetComponent())==SelfTestDoorIndex;
    bSelfTestPassed &= bDoor;
    UE_LOG(LogTemp,Display,TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] PROP_DOOR%d trace=%s"),SelfTestDoorIndex,bDoor?TEXT("PASS"):TEXT("FAIL"));
    UPrimitiveComponent* Target = Hit.GetComponent();
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,[this,bDoor,Target]() {
        if (bDoor) SelfTestRoom->Interact(Target);
        else SelfTestRoom->SetPropDoorOpen(SelfTestDoorIndex,true);
        FTimerHandle Next;
        GetWorldTimerManager().SetTimer(Next,this,&AHumanityTrinityRebuildGameMode::SelfTestValidatePropDoorOpen,1.7f,false);
    },0.6f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestValidatePropDoorOpen()
{
    const bool bOpen = SelfTestRoom->GetPropDoorOpenFraction(SelfTestDoorIndex)>.99f;
    bSelfTestPassed &= bOpen;
    UE_LOG(LogTemp,Display,TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] PROP_DOOR%d open=%s"),SelfTestDoorIndex,bOpen?TEXT("PASS"):TEXT("FAIL"));
    CaptureSelfTestScreenshot(FString::Printf(TEXT("HumanityTrinityRebuild_11_PropDoor%d_Open.png"),SelfTestDoorIndex));
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,this,&AHumanityTrinityRebuildGameMode::SelfTestEnterPropRoom,0.7f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestEnterPropRoom()
{
    const auto& Spec = HumanityPropDoors[SelfTestDoorIndex];
    const FVector Centre = Spec.Hinge+FRotator(0,Spec.Yaw,0).RotateVector(FVector(Spec.Width/2,0,0));
    // Sweep the actual player capsule through the hole, including the threshold.
    SelfTestPlayer->SetActorLocation(Centre+Spec.StageNormal*80+FVector(0,0,94),false,nullptr,ETeleportType::TeleportPhysics);
    FHitResult Hit;
    SelfTestPlayer->SetActorLocation(Centre-Spec.StageNormal*85+FVector(0,0,94),true,&Hit);
    const bool bEntered = !Hit.bBlockingHit && FVector::DotProduct(SelfTestPlayer->GetActorLocation()-Centre,Spec.StageNormal)<-70;
    bSelfTestPassed &= bEntered;
    UE_LOG(LogTemp,Display,TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] PROP_DOOR%d capsule_entry=%s blocker=%s"),SelfTestDoorIndex,
        bEntered?TEXT("PASS"):TEXT("FAIL"),Hit.GetActor()?*Hit.GetActor()->GetName():TEXT("none"));
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,this,&AHumanityTrinityRebuildGameMode::SelfTestExitPropRoom,1.0f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestExitPropRoom()
{
    const auto& Spec = HumanityPropDoors[SelfTestDoorIndex];
    const FVector Centre = Spec.Hinge+FRotator(0,Spec.Yaw,0).RotateVector(FVector(Spec.Width/2,0,0));
    SelfTestPlayer->GetController()->SetControlRotation(Spec.StageNormal.Rotation());
    // Walk from the lower interior floor up both transitions with the real
    // character movement component: no teleport, raised capsule or jump.
    bSelfTestWalking = true;
    SelfTestWalkInput();
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,this,&AHumanityTrinityRebuildGameMode::SelfTestFinishPropWalkOut,1.2f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestFinishPropWalkOut()
{
    bSelfTestWalking = false;
    const auto& Spec = HumanityPropDoors[SelfTestDoorIndex];
    const FVector Centre = Spec.Hinge+FRotator(0,Spec.Yaw,0).RotateVector(FVector(Spec.Width/2,0,0));
    const float Distance = FVector::DotProduct(SelfTestPlayer->GetActorLocation()-Centre,Spec.StageNormal);
    const bool bExited = Distance>60 && SelfTestPlayer->GetActorLocation().Z>132;
    bSelfTestPassed &= bExited;
    UE_LOG(LogTemp,Display,TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] PROP_DOOR%d walk_up_steps=%s distance=%.1f z=%.1f"),
        SelfTestDoorIndex,bExited?TEXT("PASS"):TEXT("FAIL"),Distance,SelfTestPlayer->GetActorLocation().Z);
    SelfTestRoom->SetPropDoorOpen(SelfTestDoorIndex,false);
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer,this,&AHumanityTrinityRebuildGameMode::SelfTestValidatePropDoorClosed,1.7f,false);
}

void AHumanityTrinityRebuildGameMode::SelfTestWalkInput()
{
    if (!bSelfTestWalking) return;
    // Movement input is consumed every frame, like a held W key. A fixed-rate
    // timer leaves empty frames and causes braking on high-refresh systems.
    SelfTestPlayer->AddMovementInput(HumanityPropDoors[SelfTestDoorIndex].StageNormal);
    GetWorldTimerManager().SetTimerForNextTick(this,&AHumanityTrinityRebuildGameMode::SelfTestWalkInput);
}

void AHumanityTrinityRebuildGameMode::SelfTestValidatePropDoorClosed()
{
    const auto& Spec = HumanityPropDoors[SelfTestDoorIndex];
    const FVector Centre = Spec.Hinge+FRotator(0,Spec.Yaw,0).RotateVector(FVector(Spec.Width/2,0,94));
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(PropDoorClosed),false,SelfTestPlayer);
    const bool bHit = GetWorld()->SweepSingleByChannel(Hit,Centre+Spec.StageNormal*80,Centre-Spec.StageNormal*80,
        FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92),Query);
    const bool bClosed = SelfTestRoom->GetPropDoorOpenFraction(SelfTestDoorIndex)<.01f && bHit && Hit.GetActor()==SelfTestRoom;
    bSelfTestPassed &= bClosed;
    UE_LOG(LogTemp,Display,TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] PROP_DOOR%d closed_blocks=%s"),SelfTestDoorIndex,bClosed?TEXT("PASS"):TEXT("FAIL"));
    if (++SelfTestDoorIndex<2) SelfTestPreparePropDoor();
    else SelfTestReportResult();
}

void AHumanityTrinityRebuildGameMode::SelfTestReportResult()
{
    if (bSelfTestPassed)
    {
        UE_LOG(LogTemp, Display, TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] PASS"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] FAIL"));
    }
    FTimerHandle Timer;
    GetWorldTimerManager().SetTimer(Timer, this, &AHumanityTrinityRebuildGameMode::SelfTestExit, 0.8f, false);
}

void AHumanityTrinityRebuildGameMode::SelfTestExit()
{
    FPlatformMisc::RequestExit(false);
}

void AHumanityTrinityRebuildGameMode::CaptureSelfTestScreenshot(const FString& FileName)
{
    if (!bCaptureSelfTestScreenshots)
    {
        return;
    }

    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Screenshots"), TEXT("HumanityTrinityRebuild"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString FullPath = FPaths::Combine(Directory, FileName);
    FScreenshotRequest::RequestScreenshot(FullPath, false, false);
    UE_LOG(LogTemp, Display, TEXT("[HUMANITY_TRINITY_REBUILD_SELFTEST] Screenshot requested: %s"), *FullPath);
}
