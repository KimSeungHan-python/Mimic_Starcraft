#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Structure/RTSGridTypes.h"
#include "RTSBuildGridPreview.generated.h"

class ARTSGridManager;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class USceneComponent;

UCLASS()
class MIMIC_STARCRAFT_API ARTSBuildGridPreview : public AActor
{
    GENERATED_BODY()

public:
    ARTSBuildGridPreview();

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    UStaticMesh* TileMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    UMaterialInterface* ValidCellMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    UMaterialInterface* InvalidCellMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    int32 PreviewPaddingCells = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    bool bColorNearbyCellsByPlaceable = false;// 전체다 초록색으로 표현됨 건물 지을때

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    UMaterialInterface* NeutralCellMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
    float ZOffset = 4.0f;

public:
    UFUNCTION(BlueprintCallable, Category = "Preview")
    void UpdateFootprint(
        ARTSGridManager* GridManager,
        FRTSGridCoord OriginCoord,
        int32 Width,
        int32 Height,
        bool bOverallPlacementValid
    );

    UFUNCTION(BlueprintCallable, Category = "Preview")
    void HidePreview();

private:
    UPROPERTY()
    TArray<UStaticMeshComponent*> TileComponents;

private:
    void EnsureTileCount(int32 RequiredCount);

    FVector GetTileScale(float CellSize) const;
};