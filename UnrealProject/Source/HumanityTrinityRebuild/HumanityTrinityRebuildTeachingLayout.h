// Generated from Blender teaching-wall parameters. Centimetres; facing wall, right is -X.
#pragma once
#include "CoreMinimal.h"
namespace HumanityTeaching {
inline const FVector Closed = FVector(-133.000000f, -28.000000f, 185.000000f);
inline const FVector Open = FVector(133.000000f, -28.000000f, 185.000000f);
inline const FVector Screen = FVector(-133.000000f, -12.000000f, 185.000000f);
inline const FVector ScreenSize = FVector(250.000000f, 1.000000f, 134.000000f);
inline constexpr float TravelSeconds = 2.000000f;
struct FPart { FVector Offset, Size; bool bMetal; };
inline const FPart Parts[] = {
    {FVector(0.000000f, 0.000000f, 0.000000f), FVector(264.000000f, 7.000000f, 142.000000f), false},
    {FVector(-130.750000f, -0.600000f, 0.000000f), FVector(2.500000f, 8.000000f, 142.000000f), true},
    {FVector(0.000000f, -0.600000f, -69.750000f), FVector(264.000000f, 8.000000f, 2.500000f), true},
    {FVector(130.750000f, -0.600000f, 0.000000f), FVector(2.500000f, 8.000000f, 142.000000f), true},
    {FVector(0.000000f, -0.600000f, 69.750000f), FVector(264.000000f, 8.000000f, 2.500000f), true},
};
}
