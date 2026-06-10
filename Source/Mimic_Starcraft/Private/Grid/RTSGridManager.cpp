#include "Grid/RTSGridManager.h"
#include "DrawDebugHelpers.h"
#include "Data/RTSBuildingData.h"
#include "Resources/RTSResourceNode.h"
#include "Resources/RTSVespeneGeyser.h"
#include "EngineUtils.h"

ARTSGridManager::ARTSGridManager()
{
    // Tick 사용 X
    PrimaryActorTick.bCanEverTick = false;
}

void ARTSGridManager::BeginPlay()
{
    Super::BeginPlay();


    //에디터에 배치된 위치에서 시작 
    GridOrigin = GetActorLocation();

    // 그리드 시작
    InitializeGrid();

    for (TActorIterator<ARTSVespeneGeyser> It(GetWorld()); It; ++It)
    {
        const FRTSGridCoord GeyserCoord = WorldToGrid(It->GetActorLocation());
        MarkVespeneGeyser(GeyserCoord, true);
    }

    for (TActorIterator<ARTSResourceNode> It(GetWorld()); It; ++It)
    {
        ARTSResourceNode* ResourceNode = *It;
        if (!ResourceNode || !ResourceNode->bBlocksBuildingPlacement)
        {
            continue;
        }

        OccupyBuildingCells(
            ResourceNode->GetGridOriginCoord(this),
            FMath::Max(1, ResourceNode->GridWidth),
            FMath::Max(1, ResourceNode->GridHeight),
            ResourceNode->GetUniqueID()
        );
    }




    if (bDrawDebugGrid)
    {
        DrawDebugGrid();
    }
}

void ARTSGridManager::InitializeGrid()
{
    //전체 셀 개수
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

//그리드 안에 있는 셀이 확실한지
bool ARTSGridManager::IsValidCoord(FRTSGridCoord Coord) const
{
    return Coord.X >= 0
        && Coord.Y >= 0
        && Coord.X < GridWidth
        && Coord.Y < GridHeight;
}

// 셀의 인덱스 가져오기
int32 ARTSGridManager::CoordToIndex(FRTSGridCoord Coord) const
{
    return Coord.Y * GridWidth + Coord.X;
}

//지면까지의 높이 재기
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
    //처음에 다 초기화
    OutCell.bBuildable = false;
    OutCell.bWalkable = false;
    OutCell.bOccupied = false;
    OutCell.OccupierId = INDEX_NONE;
    OutCell.bHasGround = false;
    OutCell.GroundZ = GridOrigin.Z;
    OutCell.GroundNormal = FVector::UpVector;

    //중심점 구하고
    const FVector Center = GridToWorldCenter(Coord);
    //중심과 지면까지의 높이 구하기 
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

    //꼭짓점 부근에서도 지면까지의 높이 체크
    const FVector SamplePoints[4] =
    {
        Center + FVector(Half,  Half, 0.0f),
        Center + FVector(Half, -Half, 0.0f),
        Center + FVector(-Half,  Half, 0.0f),
        Center + FVector(-Half, -Half, 0.0f)
    };

    //셀 안 각 꼭짓점들 체크
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

// 월드좌표를 그리드의 위치로 전환(인덱스)
FRTSGridCoord ARTSGridManager::WorldToGrid(FVector WorldLocation) const
{
    const FVector Local = WorldLocation - GridOrigin;

    const int32 X = FMath::FloorToInt(Local.X / CellSize);
    const int32 Y = FMath::FloorToInt(Local.Y / CellSize);

    return FRTSGridCoord(X, Y);
}

//그리드의 개수 더하기 + 셀 중간 값 해당 => 해당 셀의 가운데 값 반환 
FVector ARTSGridManager::GridToWorldCenter(FRTSGridCoord Coord) const
{
    const float WorldX = GridOrigin.X + Coord.X * CellSize + CellSize * 0.5f;
    const float WorldY = GridOrigin.Y + Coord.Y * CellSize + CellSize * 0.5f;

    return FVector(WorldX, WorldY, GridOrigin.Z);
}

//해당 셀이 땅위에 있는것인지 체크
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
//지을려는 빌딩 셀들의 중앙값이 건물이 바닥위에 있는건지 확인
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

bool ARTSGridManager::IsCellWalkable(FRTSGridCoord Coord) const
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
        && Cell.bWalkable
        && !Cell.bOccupied;
}

bool ARTSGridManager::CanPlaceBuilding(FRTSGridCoord OriginCoord, int32 Width, int32 Height) const
{
    if (Width <= 0 || Height <= 0)
    {
        return false;
    }

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

//빌딩 셀을 제거 나중에 빌딩 제거 할때 사용하면 됨.
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

        //그려지는 셀들 가로 줄 여러개 생김 
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
        //그려지는 셀들 세로 줄 여러개 생김 
        DrawDebugLine(GetWorld(), Start, End, FColor::Blue, true, -1.0f, 0, 1.0f);
    }
}

bool ARTSGridManager::CanPlaceBuildingByData(FRTSGridCoord OriginCoord, URTSBuildingData* BuildingData) const
{
    if (!BuildingData)
    {
        return false;
    }

    const int32 Width = BuildingData->GridWidth;
    const int32 Height = BuildingData->GridHeight;

    if (!CanPlaceBuilding(OriginCoord, Width, Height))
    {
        return false;
    }

    if (BuildingData->bRequiresPower)
    {
        if (!IsFootprintPowered(OriginCoord, Width, Height))
        {
            return false;
        }
    }

    if (BuildingData->bRequiresCreep)
    {
        if (!IsFootprintOnCreep(OriginCoord, Width, Height))
        {
            return false;
        }
    }

    if (BuildingData->bMustBuildOnVespeneGeyser)
    {
        if (!IsFootprintOnVespeneGeyser(OriginCoord, Width, Height))
        {
            return false;
        }
    }
    else if (DoesFootprintOverlapVespeneGeyser(OriginCoord, Width, Height))
    {
        return false;
    }

    return true;
}

//지금은 건물의 중심이 셀에 있는지 인데 전체셀로 수정해야됨
bool ARTSGridManager::IsFootprintPowered(FRTSGridCoord OriginCoord, int32 Width, int32 Height) const
{
    const FRTSGridCoord CenterCoord(
        OriginCoord.X + Width / 2,
        OriginCoord.Y + Height / 2
    );

    if (!IsValidCoord(CenterCoord))
    {
        return false;
    }

    const int32 Index = CoordToIndex(CenterCoord);

    if (!Cells.IsValidIndex(Index))
    {
        return false;
    }

    return Cells[Index].PowerProviderCount > 0;
}

bool ARTSGridManager::IsFootprintOnCreep(FRTSGridCoord OriginCoord, int32 Width, int32 Height) const
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
                return false;
            }

            const int32 Index = CoordToIndex(Coord);

            if (!Cells.IsValidIndex(Index))
            {
                return false;
            }

            if (!Cells[Index].bHasCreep)
            {
                return false;
            }
        }
    }

    return true;
}

bool ARTSGridManager::DoesFootprintOverlapVespeneGeyser(FRTSGridCoord OriginCoord, int32 Width, int32 Height) const
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
                return false;
            }

            const int32 Index = CoordToIndex(Coord);

            if (!Cells.IsValidIndex(Index))
            {
                return false;
            }

            if (Cells[Index].bHasVespeneGeyser)
            {
                return true;
            }
        }
    }

    return false;
}

bool ARTSGridManager::IsFootprintOnVespeneGeyser(FRTSGridCoord OriginCoord, int32 Width, int32 Height) const
{
    const FRTSGridCoord CenterCoord(
        OriginCoord.X + Width / 2,
        OriginCoord.Y + Height / 2
    );

    if (!IsValidCoord(CenterCoord))
    {
        return false;
    }

    const int32 Index = CoordToIndex(CenterCoord);

    if (!Cells.IsValidIndex(Index))
    {
        return false;
    }

    const FRTSGridCell& Cell = Cells[Index];

    return Cell.bHasVespeneGeyser && !Cell.bVespeneOccupied;
}

void ARTSGridManager::AddPowerInRadius(FRTSGridCoord CenterCoord, int32 RadiusCells)
{
    for (int32 Y = -RadiusCells; Y <= RadiusCells; ++Y)
    {
        for (int32 X = -RadiusCells; X <= RadiusCells; ++X)
        {
            const int32 DistSq = X * X + Y * Y;

            if (DistSq > RadiusCells * RadiusCells)
            {
                continue;
            }

            const FRTSGridCoord Coord(
                CenterCoord.X + X,
                CenterCoord.Y + Y
            );

            if (!IsValidCoord(Coord))
            {
                continue;
            }

            const int32 Index = CoordToIndex(Coord);

            if (Cells.IsValidIndex(Index))
            {
                Cells[Index].PowerProviderCount++;
            }
        }
    }
}

void ARTSGridManager::RemovePowerInRadius(FRTSGridCoord CenterCoord, int32 RadiusCells)
{
    for (int32 Y = -RadiusCells; Y <= RadiusCells; ++Y)
    {
        for (int32 X = -RadiusCells; X <= RadiusCells; ++X)
        {
            const int32 DistSq = X * X + Y * Y;

            if (DistSq > RadiusCells * RadiusCells)
            {
                continue;
            }

            const FRTSGridCoord Coord(
                CenterCoord.X + X,
                CenterCoord.Y + Y
            );

            if (!IsValidCoord(Coord))
            {
                continue;
            }

            const int32 Index = CoordToIndex(Coord);

            if (Cells.IsValidIndex(Index))
            {
                Cells[Index].PowerProviderCount = FMath::Max(
                    0,
                    Cells[Index].PowerProviderCount - 1
                );
            }
        }
    }
}

void ARTSGridManager::AddCreepInRadius(FRTSGridCoord CenterCoord, int32 RadiusCells)
{
    for (int32 Y = -RadiusCells; Y <= RadiusCells; ++Y)
    {
        for (int32 X = -RadiusCells; X <= RadiusCells; ++X)
        {
            const int32 DistSq = X * X + Y * Y;

            if (DistSq > RadiusCells * RadiusCells)
            {
                continue;
            }

            const FRTSGridCoord Coord(
                CenterCoord.X + X,
                CenterCoord.Y + Y
            );

            if (!IsValidCoord(Coord))
            {
                continue;
            }

            const int32 Index = CoordToIndex(Coord);

            if (Cells.IsValidIndex(Index))
            {
                Cells[Index].bHasCreep = true;
            }
        }
    }
}

void ARTSGridManager::MarkVespeneGeyser(FRTSGridCoord Coord, bool bHasGeyser)
{
    if (!IsValidCoord(Coord))
    {
        return;
    }

    const int32 Index = CoordToIndex(Coord);

    if (!Cells.IsValidIndex(Index))
    {
        return;
    }

    Cells[Index].bHasVespeneGeyser = bHasGeyser;

    if (!bHasGeyser)
    {
        Cells[Index].bVespeneOccupied = false;
    }
}

void ARTSGridManager::SetVespeneOccupied(FRTSGridCoord Coord, bool bOccupied)
{
    if (!IsValidCoord(Coord))
    {
        return;
    }

    const int32 Index = CoordToIndex(Coord);

    if (!Cells.IsValidIndex(Index))
    {
        return;
    }

    if (!Cells[Index].bHasVespeneGeyser)
    {
        return;
    }

    Cells[Index].bVespeneOccupied = bOccupied;
}
