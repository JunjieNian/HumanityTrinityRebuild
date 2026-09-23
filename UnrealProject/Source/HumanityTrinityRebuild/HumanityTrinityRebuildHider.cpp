#include "HumanityTrinityRebuildHider.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"
#include "UObject/ConstructorHelpers.h"

AHumanityTrinityRebuildHider::AHumanityTrinityRebuildHider()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HiderBody"));
    Body->InitCapsuleSize(28.0f, 84.0f);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Body->SetCollisionResponseToAllChannels(ECR_Block);
    RootComponent = Body;

    Silhouette = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HiderSilhouette"));
    Silhouette->SetupAttachment(Body);
    Silhouette->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Silhouette->SetRelativeScale3D(FVector(0.42f, 0.42f, 1.6f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded())
    {
        Silhouette->SetStaticMesh(Cylinder.Object);
    }
    Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HiderHead"));
    Head->SetupAttachment(Body);
    Head->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Head->SetRelativeLocation(FVector(0.0f, 0.0f, 85.0f));
    Head->SetRelativeScale3D(FVector(0.36f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded())
    {
        Head->SetStaticMesh(Sphere.Object);
    }
}

void AHumanityTrinityRebuildHider::BeginPlay()
{
    Super::BeginPlay();
    FootstepSound = LoadObject<USoundWave>(nullptr,
        TEXT("/Game/HumanityTrinityRebuild/Audio/SW_HiderFootstep.SW_HiderFootstep"));
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/HumanityTrinityRebuild/Materials/M_Hider.M_Hider")))
    {
        Silhouette->SetMaterial(0, Material);
        Head->SetMaterial(0, Material);
    }
    FootstepAttenuation = NewObject<USoundAttenuation>(this);
    FSoundAttenuationSettings& Settings = FootstepAttenuation->Attenuation;
    Settings.bAttenuate = true;
    Settings.bSpatialize = true;
    Settings.AttenuationShape = EAttenuationShape::Sphere;
    Settings.AttenuationShapeExtents = FVector(150.0f, 0.0f, 0.0f);
    Settings.FalloffDistance = 1300.0f;
    Settings.bEnableOcclusion = true;
    Settings.OcclusionTraceChannel = ECC_Visibility;
    Settings.OcclusionVolumeAttenuation = 0.35f;
    UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] HIDER_READY audio=%s position=%s"),
        FootstepSound ? TEXT("YES") : TEXT("NO"), *GetActorLocation().ToString());
}

void AHumanityTrinityRebuildHider::SetSeeker(AActor* InSeeker)
{
    Seeker = InSeeker;
}

void AHumanityTrinityRebuildHider::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Seeker)
    {
        return;
    }

    const float Separation = FVector::Dist2D(GetActorLocation(), Seeker->GetActorLocation());
    if (!bMoving)
    {
        StillSeconds -= DeltaSeconds;
        if (Separation < 280.0f || StillSeconds <= 0.0f)
        {
            // The side passage lets the hider risk moving between two pockets.
            // Waiting is a real choice: it produces no footstep cue.
            TargetY = GetActorLocation().Y < -850.0f ? -640.0f : -1110.0f;
            if (Separation < 280.0f)
            {
                TargetY = Seeker->GetActorLocation().Y > GetActorLocation().Y ? -1110.0f : -640.0f;
            }
            bMoving = true;
            UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] HIDER_MOVING target_y=%.0f"), TargetY);
        }
        return;
    }

    const float CurrentY = GetActorLocation().Y;
    const float Remaining = TargetY - CurrentY;
    if (FMath::Abs(Remaining) < 12.0f)
    {
        bMoving = false;
        StillSeconds = FMath::FRandRange(9.0f, 17.0f);
        UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] HIDER_STILL steps=%d"), StepsEmitted);
        return;
    }

    FHitResult Hit;
    const FVector Delta(0.0f, FMath::Sign(Remaining) * FMath::Min(75.0f * DeltaSeconds, FMath::Abs(Remaining)), 0.0f);
    SetActorLocation(GetActorLocation() + Delta, true, &Hit);
    if (Hit.bBlockingHit)
    {
        bMoving = false;
        StillSeconds = 4.0f;
        UE_LOG(LogTemp, Display, TEXT("[HIDE_AND_SEEK] HIDER_BLOCKED at=%s"), *GetActorLocation().ToString());
        return;
    }

    StepSeconds -= DeltaSeconds;
    if (StepSeconds <= 0.0f)
    {
        StepSeconds = 0.58f;
        ++StepsEmitted;
        if (FootstepSound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, FootstepSound, GetActorLocation(),
                1.0f, FMath::FRandRange(0.94f, 1.06f), 0.0f, FootstepAttenuation);
        }
        UE_LOG(LogTemp, Verbose, TEXT("[HIDE_AND_SEEK] FOOTSTEP %d"), StepsEmitted);
    }
}
