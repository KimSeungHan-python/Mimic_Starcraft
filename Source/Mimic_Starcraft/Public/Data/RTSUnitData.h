#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputCoreTypes.h"
#include "RTSUnitData.generated.h"

class ARTSUnitBase;
class URTSBuildingData;

UCLASS(BlueprintType, Blueprintable)
class MIMIC_STARCRAFT_API URTSUnitData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    FName UnitId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    TSubclassOf<ARTSUnitBase> UnitClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Command")
    FKey CommandHotkey;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Production")
    float BuildTime = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Cost")
    int32 MineralCost = 50;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Cost")
    int32 VespeneCost = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Supply")
    int32 SupplyCost = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Role")
    bool bWorker = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Worker", meta = (EditCondition = "bWorker"))
    TArray<TObjectPtr<URTSBuildingData>> BuildableBuildings;
};
