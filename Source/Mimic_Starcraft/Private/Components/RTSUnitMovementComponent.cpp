#include "Components/RTSUnitMovementComponent.h"

#include "Algo/Reverse.h"
#include "Components/RTSHealthComponent.h"
#include "EngineUtils.h"
#include "Grid/RTSGridManager.h"
#include "Spatial/RTSActorSpatialIndex.h"
#include "Units/RTSUnitBase.h"

URTSUnitMovementComponent::URTSUnitMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URTSUnitMovementComponent::BeginPlay()
{
    Super::BeginPlay();

    ResolveGridManager();
    SetComponentTickEnabled(GetOwner() && GetOwner()->HasAuthority() && bHasMoveCommand);
}

void URTSUnitMovementComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner() || !GetOwner()->HasAuthority() || !bHasMoveCommand)
    {
        return;
    }

    MoveAlongPath(DeltaTime);
}

void URTSUnitMovementComponent::IssueMoveCommand(const FVector& TargetLocation)
{
    ARTSUnitBase* Unit = GetOwningUnit();
    if (!Unit || !Unit->HasAuthority())
    {
        return;
    }

    if (bHasMoveCommand
        && PathPoints.Num() > 0
        && FVector::DistSquared2D(TargetLocation, MoveTargetLocation) <= FMath::Square(RepathTargetChangeThreshold))
    {
        return;
    }

    MoveTargetLocation = TargetLocation;
    bHasMoveCommand = true;
    CurrentPathIndex = 0;

    if (!BuildPathTo(TargetLocation))
    {
        PathPoints.Reset();
        PathPoints.Add(TargetLocation);
    }

    SyncOwnerMoveState(true);
    SetComponentTickEnabled(true);
}

void URTSUnitMovementComponent::StopMovement()
{
    AActor* Owner = GetOwner();
    if (Owner && !Owner->HasAuthority())
    {
        return;
    }

    bHasMoveCommand = false;
    PathPoints.Reset();
    CurrentPathIndex = 0;
    SyncOwnerMoveState(false);
    SetComponentTickEnabled(false);
}

ARTSUnitBase* URTSUnitMovementComponent::GetOwningUnit() const
{
    return Cast<ARTSUnitBase>(GetOwner());
}

void URTSUnitMovementComponent::ResolveGridManager()
{
    if (GridManager || !GetWorld())
    {
        return;
    }

    for (TActorIterator<ARTSGridManager> It(GetWorld()); It; ++It)
    {
        GridManager = *It;
        return;
    }
}

bool URTSUnitMovementComponent::BuildPathTo(const FVector& TargetLocation)
{
    ResolveGridManager();

    ARTSUnitBase* Unit = GetOwningUnit();
    if (!bUseGridPathfinding || !GridManager || !Unit)
    {
        return false;
    }

    const FRTSGridCoord StartCoord = GridManager->WorldToGrid(Unit->GetActorLocation());
    FRTSGridCoord GoalCoord = GridManager->WorldToGrid(TargetLocation);

    if (!GridManager->IsCellWalkable(StartCoord))
    {
        return false;
    }

    if (!GridManager->IsCellWalkable(GoalCoord) && !FindNearestWalkableCoord(GoalCoord, GoalCoord))
    {
        return false;
    }

    TArray<FRTSGridCoord> PathCoords;
    if (!FindPathCells(StartCoord, GoalCoord, PathCoords))
    {
        return false;
    }

    PathPoints.Reset(PathCoords.Num());

    for (int32 Index = 1; Index < PathCoords.Num(); ++Index)
    {
        FVector WorldPoint;
        if (GridManager->GetCellWorldCenterOnGround(PathCoords[Index], WorldPoint))
        {
            PathPoints.Add(WorldPoint);
        }
    }

    if (PathPoints.Num() == 0)
    {
        FVector GoalWorldPoint;
        if (GridManager->GetCellWorldCenterOnGround(GoalCoord, GoalWorldPoint))
        {
            PathPoints.Add(GoalWorldPoint);
        }
    }

    return PathPoints.Num() > 0;
}

bool URTSUnitMovementComponent::FindPathCells(
    FRTSGridCoord StartCoord,
    FRTSGridCoord GoalCoord,
    TArray<FRTSGridCoord>& OutPath
) const
{
    OutPath.Reset();

    if (!GridManager || !GridManager->IsCellWalkable(StartCoord) || !GridManager->IsCellWalkable(GoalCoord))
    {
        return false;
    }

    TArray<FRTSGridCoord> OpenSet;
    TSet<FRTSGridCoord> ClosedSet;
    TMap<FRTSGridCoord, FRTSGridCoord> CameFrom;
    TMap<FRTSGridCoord, float> CostSoFar;

    auto Heuristic = [](FRTSGridCoord A, FRTSGridCoord B)
    {
        const int32 DX = FMath::Abs(A.X - B.X);
        const int32 DY = FMath::Abs(A.Y - B.Y);
        return static_cast<float>(FMath::Max(DX, DY));
    };

    OpenSet.Add(StartCoord);
    CostSoFar.Add(StartCoord, 0.0f);

    static const FRTSGridCoord NeighborOffsets[8] =
    {
        FRTSGridCoord(1, 0),
        FRTSGridCoord(-1, 0),
        FRTSGridCoord(0, 1),
        FRTSGridCoord(0, -1),
        FRTSGridCoord(1, 1),
        FRTSGridCoord(1, -1),
        FRTSGridCoord(-1, 1),
        FRTSGridCoord(-1, -1)
    };

    int32 Iterations = 0;
    while (OpenSet.Num() > 0 && Iterations < MaxPathSearchCells)
    {
        ++Iterations;

        int32 BestIndex = 0;
        float BestScore = TNumericLimits<float>::Max();

        for (int32 Index = 0; Index < OpenSet.Num(); ++Index)
        {
            const FRTSGridCoord Coord = OpenSet[Index];
            const float* Cost = CostSoFar.Find(Coord);
            const float Score = (Cost ? *Cost : TNumericLimits<float>::Max()) + Heuristic(Coord, GoalCoord);
            if (Score < BestScore)
            {
                BestScore = Score;
                BestIndex = Index;
            }
        }

        const FRTSGridCoord Current = OpenSet[BestIndex];
        OpenSet.RemoveAtSwap(BestIndex);

        if (Current == GoalCoord)
        {
            FRTSGridCoord PathCoord = GoalCoord;
            OutPath.Add(PathCoord);

            while (PathCoord != StartCoord)
            {
                const FRTSGridCoord* ParentCoord = CameFrom.Find(PathCoord);
                if (!ParentCoord)
                {
                    return false;
                }

                PathCoord = *ParentCoord;
                OutPath.Add(PathCoord);
            }

            Algo::Reverse(OutPath);
            return true;
        }

        ClosedSet.Add(Current);

        for (const FRTSGridCoord& Offset : NeighborOffsets)
        {
            const FRTSGridCoord Next(Current.X + Offset.X, Current.Y + Offset.Y);
            if (ClosedSet.Contains(Next) || !GridManager->IsCellWalkable(Next))
            {
                continue;
            }

            if (!IsDiagonalMoveAllowed(Current, Next))
            {
                continue;
            }

            const bool bDiagonal = Offset.X != 0 && Offset.Y != 0;
            const float StepCost = bDiagonal ? 1.4142f : 1.0f;
            const float CurrentCost = CostSoFar.FindRef(Current);
            const float NewCost = CurrentCost + StepCost;

            float* ExistingCost = CostSoFar.Find(Next);
            if (!ExistingCost || NewCost < *ExistingCost)
            {
                CostSoFar.Add(Next, NewCost);
                CameFrom.Add(Next, Current);
                OpenSet.AddUnique(Next);
            }
        }
    }

    return false;
}

bool URTSUnitMovementComponent::FindNearestWalkableCoord(
    FRTSGridCoord DesiredCoord,
    FRTSGridCoord& OutCoord
) const
{
    if (!GridManager)
    {
        return false;
    }

    if (GridManager->IsCellWalkable(DesiredCoord))
    {
        OutCoord = DesiredCoord;
        return true;
    }

    for (int32 Radius = 1; Radius <= NearestWalkableSearchRadiusCells; ++Radius)
    {
        for (int32 Y = -Radius; Y <= Radius; ++Y)
        {
            for (int32 X = -Radius; X <= Radius; ++X)
            {
                if (FMath::Max(FMath::Abs(X), FMath::Abs(Y)) != Radius)
                {
                    continue;
                }

                const FRTSGridCoord Candidate(DesiredCoord.X + X, DesiredCoord.Y + Y);
                if (GridManager->IsCellWalkable(Candidate))
                {
                    OutCoord = Candidate;
                    return true;
                }
            }
        }
    }

    return false;
}

bool URTSUnitMovementComponent::IsDiagonalMoveAllowed(FRTSGridCoord FromCoord, FRTSGridCoord ToCoord) const
{
    if (!GridManager)
    {
        return false;
    }

    const int32 DX = ToCoord.X - FromCoord.X;
    const int32 DY = ToCoord.Y - FromCoord.Y;
    if (DX == 0 || DY == 0)
    {
        return true;
    }

    return GridManager->IsCellWalkable(FRTSGridCoord(FromCoord.X + DX, FromCoord.Y))
        && GridManager->IsCellWalkable(FRTSGridCoord(FromCoord.X, FromCoord.Y + DY));
}

FVector URTSUnitMovementComponent::GetCurrentMoveTarget() const
{
    if (PathPoints.IsValidIndex(CurrentPathIndex))
    {
        return PathPoints[CurrentPathIndex];
    }

    return MoveTargetLocation;
}

FVector URTSUnitMovementComponent::GetSeparationDirection() const
{
    ARTSUnitBase* Unit = GetOwningUnit();
    if (!bUseSimpleSeparation || !Unit || SeparationRadius <= KINDA_SMALL_NUMBER || !GetWorld())
    {
        return FVector::ZeroVector;
    }

    ARTSActorSpatialIndex* SpatialIndex = ARTSActorSpatialIndex::GetOrCreate(GetWorld());
    if (!SpatialIndex)
    {
        return FVector::ZeroVector;
    }

    TArray<AActor*> NearbyActors;
    SpatialIndex->QueryActorsInRadius(Unit->GetActorLocation(), SeparationRadius, NearbyActors);

    FVector Separation = FVector::ZeroVector;
    for (AActor* NearbyActor : NearbyActors)
    {
        ARTSUnitBase* OtherUnit = Cast<ARTSUnitBase>(NearbyActor);
        if (!OtherUnit || OtherUnit == Unit || OtherUnit->TeamNumber != Unit->TeamNumber)
        {
            continue;
        }

        const URTSHealthComponent* HealthComponent = OtherUnit->HealthComponent;
        if (HealthComponent && !HealthComponent->IsAlive())
        {
            continue;
        }

        FVector Away = Unit->GetActorLocation() - OtherUnit->GetActorLocation();
        Away.Z = 0.0f;

        const float Distance = Away.Size();
        if (Distance <= KINDA_SMALL_NUMBER || Distance > SeparationRadius)
        {
            continue;
        }

        Away /= Distance;
        Separation += Away * (1.0f - Distance / SeparationRadius);
    }

    return Separation.GetClampedToMaxSize(1.0f);
}

void URTSUnitMovementComponent::MoveAlongPath(float DeltaTime)
{
    ARTSUnitBase* Unit = GetOwningUnit();
    if (!Unit)
    {
        StopMovement();
        return;
    }

    while (PathPoints.IsValidIndex(CurrentPathIndex)
        && Unit->HasReachedLocation(PathPoints[CurrentPathIndex], Unit->MovementAcceptanceRadius))
    {
        ++CurrentPathIndex;
    }

    if (!PathPoints.IsValidIndex(CurrentPathIndex))
    {
        StopMovement();
        return;
    }

    const FVector CurrentLocation = Unit->GetActorLocation();
    const FVector SegmentTarget = GetCurrentMoveTarget();
    FVector Direction = SegmentTarget - CurrentLocation;
    Direction.Z = 0.0f;

    const float Distance = Direction.Size();
    if (Distance <= Unit->MovementAcceptanceRadius)
    {
        ++CurrentPathIndex;
        return;
    }

    Direction /= Distance;

    const FVector Separation = GetSeparationDirection();
    if (!Separation.IsNearlyZero())
    {
        Direction = (Direction + Separation * SeparationStrength).GetSafeNormal();
    }

    const float Step = FMath::Min(Distance, Unit->MovementSpeed * DeltaTime);
    FVector NewLocation = CurrentLocation + Direction * Step;
    NewLocation.Z = FMath::FInterpTo(CurrentLocation.Z, SegmentTarget.Z, DeltaTime, 10.0f);

    FHitResult Hit;
    Unit->SetActorLocation(NewLocation, true, &Hit);

    if (!Direction.IsNearlyZero())
    {
        Unit->SetActorRotation(Direction.Rotation());
    }
}

void URTSUnitMovementComponent::SyncOwnerMoveState(bool bMoving)
{
    ARTSUnitBase* Unit = GetOwningUnit();
    if (!Unit)
    {
        return;
    }

    Unit->bHasMoveTarget = bMoving;
    Unit->MoveTargetLocation = bMoving ? MoveTargetLocation : FVector::ZeroVector;
}
