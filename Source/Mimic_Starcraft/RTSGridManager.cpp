#include "RTSGridManager.h"
#include "DrawDebugHelpers.h"

ARTSGridManager::ARTSGridManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ARTSGridManager::BeginPlay()
{
    Super::BeginPlay();

    InitializeGrid();

    if (bDrawDebugGrid)
    {
        DrawDebugGrid();
    }
}

void ARTSGridManager::InitializeGrid()
{
    const int32 TotalCells = GridWidth * GridHeight;

    Cells.Empty();
    Cells.SetNum(TotalCells);

    for (int32 Y = 0; Y < GridHeight; ++Y)
    {
        for (int32 X = 0; X < GridWidth; ++X)
        {
            const FRTSGridCoord Coord(X, Y);
            const int32 Index = CoordToIndex(Coord);

            FRTSGridCell NewCell;
            EvaluateCellTerrain(Coord, NewCell);

            Cells[Index] = NewCell;
        }
    }
}

bool ARTSGridManager::IsValidCoord(FRTSGridCoord Coord) const
{
    return Coord.X >= 0
        && Coord.Y >= 0
        && Coord.X < GridWidth
        && Coord.Y < GridHeight;
}

int32 ARTSGridManager::CoordToIndex(FRTSGridCoord Coord) const
{
    return Coord.Y * GridWidth + Coord.X;
}

bool ARTSGridManager::TraceGroundAt(FVector XYWorldLocation, FHitResult& OutHit) const
{
    if (!GetWorld())
    {
        return false;
    }

    const FVector Start(
        XYWorldLocation.X,
        XYWorldLocation.Y,
        GridOrigin.Z + TerrainTraceStartHeight
    );

    const FVector End(
        XYWorldLocation.X,
        XYWorldLocation.Y,
        GridOrigin.Z - TerrainTraceEndDepth
    );

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    return GetWorld()->LineTraceSingleByChannel(
        OutHit,
        Start,
        End,
        TerrainTraceChannel,
        Params
    );
}

bool ARTSGridManager::EvaluateCellTerrain(FRTSGridCoord Coord, FRTSGridCell& OutCell) const
{
    OutCell.bBuildable = false;
    OutCell.bWalkable = false;
    OutCell.bOccupied = false;
    OutCell.OccupierId = INDEX_NONE;
    OutCell.bHasGround = false;
    OutCell.GroundZ = GridOrigin.Z;
    OutCell.GroundNormal = FVector::UpVector;

    const FVector Center = GridToWorldCenter(Coord);

    FHitResult CenterHit;
    if (!TraceGroundAt(Center, CenterHit))
    {
        return false;
    }

    OutCell.bHasGround = true;
    OutCell.GroundZ = CenterHit.Location.Z;
    OutCell.GroundNormal = CenterHit.ImpactNormal;

    const float UpDot = FVector::DotProduct(CenterHit.ImpactNormal.GetSafeNormal(), FVector::UpVector);
    const float ClampedDot = FMath::Clamp(UpDot, -1.0f, 1.0f);
    const float SlopeDegrees = FMath::RadiansToDegrees(FMath::Acos(ClampedDot));

    // 경사면이면 건설 불가
    if (SlopeDegrees > MaxBuildSlopeDegrees)
    {
        return false;
    }

    // 평평한 고지대까지 전부 금지하고 싶을 때
    if (bUseMaxBuildWorldZ && CenterHit.Location.Z > MaxBuildWorldZ)
    {
        return false;
    }

    float MinZ = CenterHit.Location.Z;
    float MaxZ = CenterHit.Location.Z;

    const float Half = CellSize * 0.45f;

    const FVector SamplePoints[4] =
    {
        Center + FVector(Half,  Half, 0.0f),
        Center + FVector(Half, -Half, 0.0f),
        Center + FVector(-Half,  Half, 0.0f),
        Center + FVector(-Half, -Half, 0.0f)
    };

    for (const FVector& SamplePoint : SamplePoints)
    {
        FHitResult Hit;
        if (!TraceGroundAt(SamplePoint, Hit))
        {
            return false;
        }

        MinZ = FMath::Min(MinZ, Hit.Location.Z);
        MaxZ = FMath::Max(MaxZ, Hit.Location.Z);
    }

    // 셀 하나 안에서도 높이 차이가 크면 언덕/경사로 판단
    if ((MaxZ - MinZ) > MaxCellHeightDelta)
    {
        return false;
    }

    OutCell.bBuildable = true;
    OutCell.bWalkable = true;

    return true;
}

FRTSGridCoord ARTSGridManager::WorldToGrid(FVector WorldLocation) const
{
    const FVector Local = WorldLocation - GridOrigin;

    const int32 X = FMath::FloorToInt(Local.X / CellSize);
    const int32 Y = FMath::FloorToInt(Local.Y / CellSize);

    return FRTSGridCoord(X, Y);
}

FVector ARTSGridManager::GridToWorldCenter(FRTSGridCoord Coord) const
{
    const float WorldX = GridOrigin.X + Coord.X * CellSize + CellSize * 0.5f;
    const float WorldY = GridOrigin.Y + Coord.Y * CellSize + CellSize * 0.5f;

    return FVector(WorldX, WorldY, GridOrigin.Z);
}

bool ARTSGridManager::GetCellWorldCenterOnGround(FRTSGridCoord Coord, FVector& OutLocation) const
{
    OutLocation = GridToWorldCenter(Coord);

    if (!IsValidCoord(Coord))
    {
        return false;
    }

    const int32 Index = CoordToIndex(Coord);

    if (!Cells.IsValidIndex(Index))
    {
        return false;
    }

    const FRTSGridCell& Cell = Cells[Index];

    if (!Cell.bHasGround)
    {
        return false;
    }

    OutLocation.Z = Cell.GroundZ;
    return true;
}

bool ARTSGridManager::GetBuildingCenterLocationOnGround(FRTSGridCoord OriginCoord, int32 Width, int32 Height, FVector& OutLocation) const
{
    OutLocation = GridToWorldCenter(OriginCoord);

    const float OffsetX = (Width - 1) * CellSize * 0.5f;
    const float OffsetY = (Height - 1) * CellSize * 0.5f;

    OutLocation.X += OffsetX;
    OutLocation.Y += OffsetY;

    float SumZ = 0.0f;
    int32 ValidGroundCount = 0;

    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            const FRTSGridCoord Coord(
                OriginCoord.X + X,
                OriginCoord.Y + Y
            );

            if (!IsValidCoord(Coord))
            {
                continue;
            }

            const int32 Index = CoordToIndex(Coord);

            if (!Cells.IsValidIndex(Index))
            {
                continue;
            }

            const FRTSGridCell& Cell = Cells[Index];

            if (!Cell.bHasGround)
            {
                continue;
            }

            SumZ += Cell.GroundZ;
            ++ValidGroundCount;
        }
    }

    if (ValidGroundCount > 0)
    {
        OutLocation.Z = SumZ / ValidGroundCount;
        return true;
    }

    OutLocation.Z = GridOrigin.Z;
    UE_LOG(LogTemp, Warning, TEXT("Building Ground Z: %f"), OutLocation.Z);
    return false;
}

bool ARTSGridManager::IsCellPlaceable(FRTSGridCoord Coord) const
{
    if (!IsValidCoord(Coord))
    {
        return false;
    }

    const int32 Index = CoordToIndex(Coord);

    if (!Cells.IsValidIndex(Index))
    {
        return false;
    }

    const FRTSGridCell& Cell = Cells[Index];

    return Cell.bHasGround
        && Cell.bBuildable
        && !Cell.bOccupied;
}

bool ARTSGridManager::CanPlaceBuilding(FRTSGridCoord OriginCoord, int32 Width, int32 Height) const
{
    float MinZ = TNumericLimits<float>::Max();
    float MaxZ = -TNumericLimits<float>::Max();

    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            const FRTSGridCoord CheckCoord(
                OriginCoord.X + X,
                OriginCoord.Y + Y
            );

            if (!IsCellPlaceable(CheckCoord))
            {
                return false;
            }

            const int32 Index = CoordToIndex(CheckCoord);
            const FRTSGridCell& Cell = Cells[Index];

            MinZ = FMath::Min(MinZ, Cell.GroundZ);
            MaxZ = FMath::Max(MaxZ, Cell.GroundZ);
        }
    }

    // 건물 전체 footprint 안에서 높이 차이가 크면 건설 불가
    if ((MaxZ - MinZ) > MaxFootprintHeightDelta)
    {
        return false;
    }

    return true;
}

void ARTSGridManager::OccupyBuildingCells(FRTSGridCoord OriginCoord, int32 Width, int32 Height, int32 OccupierId)
{
    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            const FRTSGridCoord Coord(
                OriginCoord.X + X,
                OriginCoord.Y + Y
            );

            if (!IsValidCoord(Coord))
            {
                continue;
            }

            const int32 Index = CoordToIndex(Coord);

            if (!Cells.IsValidIndex(Index))
            {
                continue;
            }

            FRTSGridCell& Cell = Cells[Index];
            Cell.bOccupied = true;
            Cell.bWalkable = false;
            Cell.OccupierId = OccupierId;
        }
    }
}

void ARTSGridManager::ReleaseBuildingCells(FRTSGridCoord OriginCoord, int32 Width, int32 Height)
{
    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            const FRTSGridCoord Coord(
                OriginCoord.X + X,
                OriginCoord.Y + Y
            );

            if (!IsValidCoord(Coord))
            {
                continue;
            }

            const int32 Index = CoordToIndex(Coord);

            if (!Cells.IsValidIndex(Index))
            {
                continue;
            }

            FRTSGridCell& Cell = Cells[Index];
            Cell.bOccupied = false;
            Cell.bWalkable = true;
            Cell.OccupierId = INDEX_NONE;
        }
    }
}

void ARTSGridManager::DrawDebugGrid()
{
    if (!GetWorld())
    {
        return;
    }

    const float Z = GridOrigin.Z + DebugDrawHeight;

    for (int32 X = 0; X <= GridWidth; ++X)
    {
        const FVector Start = FVector(
            GridOrigin.X + X * CellSize,
            GridOrigin.Y,
            Z
        );

        const FVector End = FVector(
            GridOrigin.X + X * CellSize,
            GridOrigin.Y + GridHeight * CellSize,
            Z
        );

        DrawDebugLine(GetWorld(), Start, End, FColor::Blue, true, -1.0f, 0, 1.0f);
    }

    for (int32 Y = 0; Y <= GridHeight; ++Y)
    {
        const FVector Start = FVector(
            GridOrigin.X,
            GridOrigin.Y + Y * CellSize,
            Z
        );

        const FVector End = FVector(
            GridOrigin.X + GridWidth * CellSize,
            GridOrigin.Y + Y * CellSize,
            Z
        );

        DrawDebugLine(GetWorld(), Start, End, FColor::Blue, true, -1.0f, 0, 1.0f);
    }
}