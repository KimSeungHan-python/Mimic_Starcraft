#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTSBuildingData.generated.h"

class ARTSBuilding;

UCLASS(BlueprintType)
class MIMIC_STARCRAFT_API URTSBuildingData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    FName BuildingId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Grid")
    int32 GridWidth = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Grid")
    int32 GridHeight = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    TSubclassOf<ARTSBuilding> BuildingClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    UStaticMesh* PreviewMesh = nullptr;
};