#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/RTSGridTypes.h"
#include "RTSGridManager.generated.h"

class ARTSBuilding;
class URTSBuildingData;

UCLASS()
class MIMIC_STARCRAFT_API ARTSGridManager : public AActor
{
    GENERATED_BODY()

public:
    ARTSGridManager();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid")
    int32 GridWidth = 128;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid")
    int32 GridHeight = 128;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid")
    float CellSize = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid")
    FVector GridOrigin = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid|Debug")
    bool bDrawDebugGrid = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid|Debug")
    float DebugDrawHeight = 5.0f;

    // 바닥을 찾기 위한 Trace 채널
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid|Terrain")
    TEnumAsByte<ECollisionChannel> TerrainTraceChannel = ECC_Visibility;

    // 위에서 아래로 쏠 Trace 높이
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid|Terrain")
    float TerrainTraceStartHeight = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid|Terrain")
    float TerrainTraceEndDepth = 5000.0f;

    // 셀 하나의 최대 허용 경사도
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid|Terrain")
    float MaxBuildSlopeDegrees = 8.0f;

    // 셀 하나 안에서 네 귀퉁이 높이 차이가 이 값보다 크면 건설 불가
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid|Terrain")
    float MaxCellHeightDelta = 15.0f;

    // 건물 footprint 전체 높이 차이 허용값
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid|Terrain")
    float MaxFootprintHeightDelta = 25.0f;

    // 평평한 고지대도 전부 건설 금지하고 싶을 때 사용
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid|Terrain")
    bool bUseMaxBuildWorldZ = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Grid|Terrain")
    float MaxBuildWorldZ = 300.0f;

private:
    UPROPERTY()
    TArray<FRTSGridCell> Cells;

public:
    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    void InitializeGrid();

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    bool IsValidCoord(FRTSGridCoord Coord) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    FRTSGridCoord WorldToGrid(FVector WorldLocation) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    FVector GridToWorldCenter(FRTSGridCoord Coord) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    bool GetCellWorldCenterOnGround(FRTSGridCoord Coord, FVector& OutLocation) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    bool GetBuildingCenterLocationOnGround(
        FRTSGridCoord OriginCoord,
        int32 Width,
        int32 Height,
        FVector& OutLocation
    ) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    bool IsCellPlaceable(FRTSGridCoord Coord) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    bool IsCellWalkable(FRTSGridCoord Coord) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    bool CanPlaceBuilding(FRTSGridCoord OriginCoord, int32 Width, int32 Height) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    void OccupyBuildingCells(FRTSGridCoord OriginCoord, int32 Width, int32 Height, int32 OccupierId);

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    void ReleaseBuildingCells(FRTSGridCoord OriginCoord, int32 Width, int32 Height);

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    void DrawDebugGrid();

    //그리드 매니저 추가 업데이트

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    bool CanPlaceBuildingByData(
        FRTSGridCoord OriginCoord,
        URTSBuildingData* BuildingData
    ) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    bool IsFootprintPowered(FRTSGridCoord OriginCoord, int32 Width, int32 Height) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    bool IsFootprintOnCreep(FRTSGridCoord OriginCoord, int32 Width, int32 Height) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    bool IsFootprintOnVespeneGeyser(FRTSGridCoord OriginCoord, int32 Width, int32 Height) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    void AddPowerInRadius(FRTSGridCoord CenterCoord, int32 RadiusCells);

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    void RemovePowerInRadius(FRTSGridCoord CenterCoord, int32 RadiusCells);

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    void AddCreepInRadius(FRTSGridCoord CenterCoord, int32 RadiusCells);

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    void MarkVespeneGeyser(FRTSGridCoord Coord, bool bHasGeyser);

    UFUNCTION(BlueprintCallable, Category = "RTS Grid")
    void SetVespeneOccupied(FRTSGridCoord Coord, bool bOccupied);

private:
    bool DoesFootprintOverlapVespeneGeyser(FRTSGridCoord OriginCoord, int32 Width, int32 Height) const;

    int32 CoordToIndex(FRTSGridCoord Coord) const;

    bool TraceGroundAt(FVector XYWorldLocation, FHitResult& OutHit) const;

    bool EvaluateCellTerrain(FRTSGridCoord Coord, FRTSGridCell& OutCell) const;
};
