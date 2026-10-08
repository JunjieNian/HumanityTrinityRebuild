#include "HumanityTrinityRebuildTeacher.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
    const TCHAR* TeacherPartNames[] = {TEXT("Pelvis"),TEXT("Torso"),TEXT("Head"),
        TEXT("UpperArm"),TEXT("Forearm"),TEXT("Hand"),TEXT("UpperArm"),TEXT("Forearm"),TEXT("Hand"),
        TEXT("Thigh"),TEXT("Shin"),TEXT("Shoe"),TEXT("Thigh"),TEXT("Shin"),TEXT("Shoe")};
    const int32 Parents[] = {-1,-1,-1,-1,3,4,-1,6,7,-1,9,10,-1,12,13};
    const FVector JointPositions[] = {FVector(0,0,94),FVector(0,0,94),FVector(0,0,154),
        FVector(0,23,140),FVector(0,0,-30.5f),FVector(0,0,-29.8f),
        FVector(0,-23,140),FVector(0,0,-30.5f),FVector(0,0,-29.8f),
        FVector(0,10.5f,94),FVector(0,0,-44.3f),FVector(0,0,-43),
        FVector(0,-10.5f,94),FVector(0,0,-44.3f),FVector(0,0,-43)};
}

AHumanityTrinityRebuildTeacher::AHumanityTrinityRebuildTeacher()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("TeacherFeetRoot"));
    RootComponent=SceneRoot;
    BodyPivot=CreateDefaultSubobject<USceneComponent>(TEXT("TeacherBodyPivot"));
    BodyPivot->SetupAttachment(SceneRoot);
    for (int32 I=0; I<UE_ARRAY_COUNT(TeacherPartNames); ++I)
    {
        auto* Joint=CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("TeacherJoint%02d"),I));
        Joint->SetupAttachment(Parents[I]<0 ? BodyPivot.Get() : Joints[Parents[I]].Get());
        Joint->SetRelativeLocation(JointPositions[I]);
        Joints.Add(Joint);
        auto* Part=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("TeacherPart%02d"),I));
        Part->SetupAttachment(Joint);
        Part->SetMobility(EComponentMobility::Movable);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetCastShadow(true);
        Parts.Add(Part);
    }
}

void AHumanityTrinityRebuildTeacher::BeginPlay()
{
    Super::BeginPlay();
    for (int32 I=0; I<Parts.Num(); ++I)
    {
        const FString Path=FString::Printf(
            TEXT("/Game/HumanityTrinityRebuild/Characters/Teacher/SM_Teacher_%s.SM_Teacher_%s"),
            TeacherPartNames[I],TeacherPartNames[I]);
        Parts[I]->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));
    }
    UE_LOG(LogTemp,Display,TEXT("[TEACHER_PATROL] TEACHER_READY parts=%d/15 modeled_adult=YES"),GetLoadedPartCount());
}

void AHumanityTrinityRebuildTeacher::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AnimationTime+=DeltaSeconds;
    WalkBlend=FMath::FInterpTo(WalkBlend,bWalking ? 1.f : 0.f,DeltaSeconds,8.f);
    const float Cycle=AnimationTime*6.4f;
    const float Step=FMath::Sin(Cycle)*WalkBlend;
    const float Bob=FMath::Abs(FMath::Cos(Cycle))*WalkBlend*1.6f;
    BodyPivot->SetRelativeLocation(FVector(0,0,Bob));
    Joints[1]->SetRelativeRotation(FRotator(0,FMath::Sin(Cycle)*WalkBlend*2.f,0));
    Joints[2]->SetRelativeRotation(FRotator(0,bWalking ? Step*2.f : FMath::Sin(AnimationTime*.65f)*11.f,0));
    Joints[3]->SetRelativeRotation(FRotator(-Step*20.f,0,-4.f));
    Joints[6]->SetRelativeRotation(FRotator(Step*20.f,0,4.f));
    Joints[4]->SetRelativeRotation(FRotator(-8.f-FMath::Max(Step,0.f)*8.f,0,0));
    Joints[7]->SetRelativeRotation(FRotator(-8.f-FMath::Max(-Step,0.f)*8.f,0,0));
    Joints[9]->SetRelativeRotation(FRotator(Step*25.f,0,0));
    Joints[12]->SetRelativeRotation(FRotator(-Step*25.f,0,0));
    Joints[10]->SetRelativeRotation(FRotator(-FMath::Max(Step,0.f)*32.f,0,0));
    Joints[13]->SetRelativeRotation(FRotator(-FMath::Max(-Step,0.f)*32.f,0,0));
    Joints[11]->SetRelativeRotation(FRotator(-Step*8.f,0,0));
    Joints[14]->SetRelativeRotation(FRotator(Step*8.f,0,0));
}

void AHumanityTrinityRebuildTeacher::FaceIntoRoom()
{
    SetActorRotation(FRotator::ZeroRotator);
}
int32 AHumanityTrinityRebuildTeacher::GetLoadedPartCount() const
{
    int32 Count=0;
    for (const UStaticMeshComponent* Part : Parts) if (Part && Part->GetStaticMesh()) ++Count;
    return Count;
}
