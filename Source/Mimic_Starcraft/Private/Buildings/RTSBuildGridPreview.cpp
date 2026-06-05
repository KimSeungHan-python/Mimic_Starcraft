#include "Buildings/RTSBuildGridPreview.h"
#include "Grid/RTSGridManager.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

ARTSBuildGridPreview::ARTSBuildGridPreview()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(
        TEXT("/Engine/BasicShapes/Plane.Plane")
    );

    if (PlaneMeshFinder.Succeeded())
    {
        TileMesh = PlaneMeshFinder.Object;
    }
}

void ARTSBuildGridPreview::EnsureTileCount(int32 RequiredCount)
{
    while (TileComponents.Num() < RequiredCount)
    {
        UStaticMeshComponent* NewTile = NewObject<UStaticMeshComponent>(this);
        NewTile->RegisterComponent();
        NewTile->AttachToComponent(
            SceneRoot,
            FAttachmentTransformRules::KeepRelativeTransform
        );

        NewTile->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        NewTile->SetGenerateOverlapEvents(false);

        if (TileMesh)
        {
            NewTile->SetStaticMesh(TileMesh);
        }

        TileComponents.Add(NewTile);
    }
}

FVector ARTSBuildGridPreview::GetTileScale(float CellSize) const
{
    if (!TileMesh)
    {
        return FVector(1.0f);
    }

    const FBoxSphereBounds Bounds = TileMesh->GetBounds();
    const FVector MeshSize = Bounds.BoxExtent * 2.0f;

    if (MeshSize.X <= KINDA_SMALL_NUMBER || MeshSize.Y <= KINDA_SMALL_NUMBER)
    {
        return FVector(1.0f);
    }

    return FVector(
        CellSize / MeshSize.X,
        CellSize / MeshSize.Y,
        1.0f
    );
}

void ARTSBuildGridPreview::UpdateFootprint(
    ARTSGridManager* GridManager,
    FRTSGridCoord OriginCoord,
    int32 Width,
    int32 Height,
    bool bOverallPlacementValid
)
{
    if (!GridManager)
    {
        HidePreview();
        return;
    }

    const int32 Padding = FMath::Max(0, PreviewPaddingCells);

    const int32 AreaWidth = Width + Padding * 2;
    const int32 AreaHeight = Height + Padding * 2;
    const int32 RequiredCount = AreaWidth * AreaHeight;

    EnsureTileCount(RequiredCount);

    const FVector TileScale = GetTileScale(GridManager->CellSize);

    int32 TileIndex = 0;

    for (int32 LocalY = -Padding; LocalY < Height + Padding; ++LocalY)
    {
        for (int32 LocalX = -Padding; LocalX < Width + Padding; ++LocalX)
        {
            if (!TileComponents.IsValidIndex(TileIndex))
            {
                continue;
            }

            UStaticMeshComponent* TileComp = TileComponents[TileIndex];

            const FRTSGridCoord Coord(
                OriginCoord.X + LocalX,
                OriginCoord.Y + LocalY
            );

            if (!GridManager->IsValidCoord(Coord))
            {
                TileComp->SetVisibility(false, true);
                ++TileIndex;
                continue;
            }

            FVector TileLocation;
            const bool bHasGround = GridManager->GetCellWorldCenterOnGround(Coord, TileLocation);

            if (!bHasGround)
            {
                TileLocation = GridManager->GridToWorldCenter(Coord);
            }

            TileLocation.Z += ZOffset;

            const bool bInsideFootprint =
                LocalX >= 0 &&
                LocalY >= 0 &&
                LocalX < Width &&
                LocalY < Height;

            const bool bCellPlaceable = GridManager->IsCellPlaceable(Coord);

            UMaterialInterface* TargetMaterial = NeutralCellMaterial;

            if (bInsideFootprint)
            {
                /*
                 * 건물이 실제로 차지할 칸.
                 * 전체 배치가 가능하면 초록,
                 * 하나라도 문제가 있으면 footprint 전체를 빨강으로 보여줌.
                 */
                TargetMaterial = bOverallPlacementValid && bCellPlaceable
                    ? ValidCellMaterial
                    : InvalidCellMaterial;
            }
            else
            {
                /*
                 * 주변 Grid.
                 * 옵션에 따라 주변 칸도 건설 가능 여부를 보여줌.
                 */
                if (bColorNearbyCellsByPlaceable)
                {
                    TargetMaterial = bCellPlaceable
                        ? ValidCellMaterial
                        : InvalidCellMaterial;
                }
                else
                {
                    TargetMaterial = NeutralCellMaterial
                        ? NeutralCellMaterial
                        : ValidCellMaterial;
                }
            }

            TileComp->SetWorldLocation(TileLocation);
            TileComp->SetWorldRotation(FRotator::ZeroRotator);
            TileComp->SetWorldScale3D(TileScale);

            if (TileMesh)
            {
                TileComp->SetStaticMesh(TileMesh);
            }

            if (TargetMaterial)
            {
                TileComp->SetMaterial(0, TargetMaterial);
            }

            TileComp->SetVisibility(true, true);

            ++TileIndex;
        }
    }

    for (int32 i = RequiredCount; i < TileComponents.Num(); ++i)
    {
        if (TileComponents[i])
        {
            TileComponents[i]->SetVisibility(false, true);
        }
    }
}

void ARTSBuildGridPreview::HidePreview()
{
    for (UStaticMeshComponent* TileComp : TileComponents)
    {
        if (TileComp)
        {
            TileComp->SetVisibility(false, true);
        }
    }
}