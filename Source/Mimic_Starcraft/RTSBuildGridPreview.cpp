#include "RTSBuildGridPreview.h"
#include "RTSGridManager.h"
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

    const int32 RequiredCount = Width * Height;
    EnsureTileCount(RequiredCount);

    const FVector TileScale = GetTileScale(GridManager->CellSize);

    int32 TileIndex = 0;

    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            if (!TileComponents.IsValidIndex(TileIndex))
            {
                continue;
            }

            UStaticMeshComponent* TileComp = TileComponents[TileIndex];

            const FRTSGridCoord Coord(
                OriginCoord.X + X,
                OriginCoord.Y + Y
            );

            FVector TileLocation;
            const bool bHasGround = GridManager->GetCellWorldCenterOnGround(Coord, TileLocation);

            if (!bHasGround)
            {
                TileLocation = GridManager->GridToWorldCenter(Coord);
            }

            TileLocation.Z += ZOffset;

            const bool bCellPlaceable = GridManager->IsCellPlaceable(Coord);

            // 전체 footprint가 불가능하면 모두 빨강.
            // 개별 셀도 불가능하면 빨강.
            const bool bShowGreen = bOverallPlacementValid && bCellPlaceable;

            TileComp->SetWorldLocation(TileLocation);
            TileComp->SetWorldRotation(FRotator::ZeroRotator);
            TileComp->SetWorldScale3D(TileScale);

            if (TileMesh)
            {
                TileComp->SetStaticMesh(TileMesh);
            }

            UMaterialInterface* TargetMaterial = bShowGreen
                ? ValidCellMaterial
                : InvalidCellMaterial;

            if (TargetMaterial)
            {
                TileComp->SetMaterial(0, TargetMaterial);
            }

            TileComp->SetVisibility(true, true);
            TileIndex++;
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