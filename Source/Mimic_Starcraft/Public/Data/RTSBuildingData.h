#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTSBuildingData.generated.h"

class ARTSBuilding;

UENUM(BlueprintType)
enum class ERTSBuildingRace : uint8
{
    Neutral,
    Terran,
    Protoss,
    Zerg
};

UCLASS(BlueprintType, Blueprintable)
class MIMIC_STARCRAFT_API URTSBuildingData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    FName BuildingId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    ERTSBuildingRace Race = ERTSBuildingRace::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Grid")
    int32 GridWidth = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Grid")
    int32 GridHeight = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    TSubclassOf<ARTSBuilding> BuildingClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    UStaticMesh* PreviewStaticMesh = nullptr;

    // 건설 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Construction")
    float BuildTime = 10.0f;

    // 테란 건물용
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Terran")
    bool bCanLiftOff = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Terran")
    float LiftOffHeight = 300.0f;

    // 프로토스 건물용
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Protoss")
    bool bRequiresPower = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Protoss")
    bool bProvidesPower = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Protoss")
    int32 PowerRadiusCells = 6;

    // 저그 건물용
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Zerg")
    bool bRequiresCreep = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Zerg")
    bool bProvidesCreep = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Zerg")
    int32 CreepRadiusCells = 5;

    // 베스핀 건물용
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Resource")
    bool bMustBuildOnVespeneGeyser = false;
};