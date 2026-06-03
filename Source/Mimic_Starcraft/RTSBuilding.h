#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Structure/RTSGridTypes.h"
#include "RTSBuilding.generated.h"

class URTSBuildingData;

UCLASS()
class MIMIC_STARCRAFT_API ARTSBuilding : public AActor
{
    GENERATED_BODY()

public:
    ARTSBuilding();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    URTSBuildingData* BuildingData;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    FRTSGridCoord GridOriginCoord;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    int32 GridWidth = 1;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    int32 GridHeight = 1;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    bool bIsPreviewBuilding = false;

    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void InitializeBuilding(
        URTSBuildingData* InBuildingData,
        FRTSGridCoord InGridOriginCoord,
        int32 InGridWidth,
        int32 InGridHeight
    );

    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void FitMeshToGridFootprint(float CellSize);

 
    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void SetPreviewBuildingMode(bool bPreview);
};