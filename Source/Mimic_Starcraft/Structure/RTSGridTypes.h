#pragma once

#include "CoreMinimal.h"
#include "RTSGridTypes.generated.h"

USTRUCT(BlueprintType)
struct FRTSGridCoord
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 X = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Y = 0;

    FRTSGridCoord() {}

    FRTSGridCoord(int32 InX, int32 InY)
        : X(InX), Y(InY)
    {
    }

    bool operator==(const FRTSGridCoord& Other) const
    {
        return X == Other.X && Y == Other.Y;
    }
};

USTRUCT(BlueprintType)
struct FRTSGridCell
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bBuildable = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bWalkable = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bOccupied = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 OccupierId = INDEX_NONE;

    // 지형이 실제로 있는지
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasGround = false;

    // 해당 셀 중앙의 바닥 높이
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GroundZ = 0.0f;

    // 해당 셀 중앙의 바닥 노멀
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector GroundNormal = FVector::UpVector;


    //그리드 타입 추가 업데이트 
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PowerProviderCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasCreep = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasVespeneGeyser = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bVespeneOccupied = false;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite)
    //float Height;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite)
    //bool bHasCreep;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite)
    //bool bHasPower;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite)
    //bool bIsResourceBlocked;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite)
    //bool bIsRamp;
};