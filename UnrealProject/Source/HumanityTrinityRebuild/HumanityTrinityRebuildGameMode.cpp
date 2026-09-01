#include "HumanityTrinityRebuildGameMode.h"

#include "HumanityTrinityRebuildEnvironmentActor.h"
#include "HumanityTrinityRebuildHUD.h"
#include "HumanityTrinityRebuildLightSwitch.h"
#include "HumanityTrinityRebuildLightingController.h"
#include "HumanityTrinityRebuildPlayerCharacter.h"
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
        // Front-door switch, on the solid wall immediately beside the opening.
        const FVector SwitchLocation(-565.0f, -95.0f, 125.0f);
        const FRotator SwitchRotation(0.0f, 0.0f, 0.0f);
        GetWorld()->SpawnActor<AHumanityTrinityRebuildLightSwitch>(
            AHumanityTrinityRebuildLightSwitch::StaticClass(),
            SwitchLocation,
            SwitchRotation);
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            2300,
            10.0f,
            FColor::Green,
            TEXT("Humanity Trinity interactive prototype loaded."));
    }

    if (FParse::Param(FCommandLine::Get(), TEXT("HumanityTrinityRebuildSelfTest")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AHumanityTrinityRebuildGameMode::StartSelfTest);
    }
}

void AHumanityTrinityRebuildGameMode::StartSelfTest()
{
    bCaptureSelfTestScreenshots = FParse::Param(FCommandLine::Get(), TEXT("HumanityTrinityRebuildCapture"));
    SelfTestPlayer = Cast<AHumanityTrinityRebuildPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
    if (!SelfTestLighting || !SelfTestPlayer)
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
    SelfTestPlayer->SetActorLocation(FVector(-360.0f, -95.0f, 96.0f), false, nullptr, ETeleportType::TeleportPhysics);
    if (AController* Controller = SelfTestPlayer->GetController())
    {
        Controller->SetControlRotation(FRotator(-10.5f, 180.0f, 0.0f));
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
