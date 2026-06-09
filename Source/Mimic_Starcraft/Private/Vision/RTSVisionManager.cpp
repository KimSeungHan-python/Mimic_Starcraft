#include "Vision/RTSVisionManager.h"

#include "Buildings/RTSBuilding.h"
#include "Core/RTSPlayerController.h"
#include "Core/RTSPlayerState.h"
#include "Data/RTSBuildingData.h"
#include "Data/RTSUnitData.h"
#include "Grid/RTSGridManager.h"
#include "Units/RTSUnitBase.h"

#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

ARTSVisionManager::ARTSVisionManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARTSVisionManager::BeginPlay()
{
    Super::BeginPlay();

    ResolveGridManager();
    RefreshVision();
}

void ARTSVisionManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TimeUntilNextUpdate -= DeltaTime;
    if (TimeUntilNextUpdate > 0.0f)
    {
        return;
    }

    TimeUntilNextUpdate = FMath::Max(0.02f, UpdateInterval);
    RefreshVision();
}

void ARTSVisionManager::RefreshVision()
{
    if (!ResolveGridManager())
    {
        return;
    }

    ResetCurrentVisibleCells();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<ARTSUnitBase> It(World); It; ++It)
    {
        ARTSUnitBase* Unit = *It;
        const int32 TeamNumber = GetActorTeamNumber(Unit);
        if (!Unit || TeamNumber < 0)
        {
            continue;
        }

        MarkActorVision(Unit, TeamNumber, GetUnitSightRadiusCells(Unit));
    }

    for (TActorIterator<ARTSBuilding> It(World); It; ++It)
    {
        ARTSBuilding* Building = *It;
        const int32 TeamNumber = GetActorTeamNumber(Building);
        if (!Building || Building->bIsPreviewBuilding || TeamNumber < 0)
        {
            continue;
        }

        MarkActorVision(Building, TeamNumber, GetBuildingSightRadiusCells(Building));
    }

    UpdateLocalVisionCache();

    if (bApplyLocalActorVisibility)
    {
        ApplyLocalActorVisibility();
    }

    if (bDrawDebugLocalVision)
    {
        DrawDebugLocalVision();
    }

    OnLocalVisionUpdated.Broadcast();
}

bool ARTSVisionManager::IsGridCoordVisibleToTeam(FRTSGridCoord Coord, int32 TeamNumber) const
{
    const FRTSTeamVisionState* VisionState = TeamVisionStates.Find(TeamNumber);
    return VisionState && VisionState->VisibleCells.Contains(CoordToVisionIndex(Coord));
}

bool ARTSVisionManager::IsGridCoordExploredByTeam(FRTSGridCoord Coord, int32 TeamNumber) const
{
    const FRTSTeamVisionState* VisionState = TeamVisionStates.Find(TeamNumber);
    return VisionState && VisionState->ExploredCells.Contains(CoordToVisionIndex(Coord));
}

bool ARTSVisionManager::IsWorldLocationVisibleToTeam(const FVector& WorldLocation, int32 TeamNumber) const
{
    return GridManager
        && IsGridCoordVisibleToTeam(GridManager->WorldToGrid(WorldLocation), TeamNumber);
}

bool ARTSVisionManager::IsWorldLocationExploredByTeam(const FVector& WorldLocation, int32 TeamNumber) const
{
    return GridManager
        && IsGridCoordExploredByTeam(GridManager->WorldToGrid(WorldLocation), TeamNumber);
}

bool ARTSVisionManager::IsActorVisibleToTeam(AActor* Actor, int32 TeamNumber) const
{
    if (!Actor)
    {
        return false;
    }

    const int32 ActorTeamNumber = GetActorTeamNumber(Actor);
    if (ActorTeamNumber == TeamNumber)
    {
        return true;
    }

    return IsWorldLocationVisibleToTeam(Actor->GetActorLocation(), TeamNumber);
}

void ARTSVisionManager::ApplyLocalActorVisibility()
{
    LocalTeamNumber = ResolveLocalTeamNumber();
    if (LocalTeamNumber < 0)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (TActorIterator<ARTSUnitBase> It(World); It; ++It)
    {
        ARTSUnitBase* Unit = *It;
        if (!Unit)
        {
            continue;
        }

        Unit->SetActorHiddenInGame(!ShouldLocallyShowActor(Unit, LocalTeamNumber));
    }

    for (TActorIterator<ARTSBuilding> It(World); It; ++It)
    {
        ARTSBuilding* Building = *It;
        if (!Building || Building->bIsPreviewBuilding)
        {
            continue;
        }

        Building->SetActorHiddenInGame(!ShouldLocallyShowActor(Building, LocalTeamNumber));
    }
}

ARTSGridManager* ARTSVisionManager::ResolveGridManager()
{
    if (GridManager)
    {
        return GridManager;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<ARTSGridManager> It(World); It; ++It)
    {
        GridManager = *It;
        break;
    }

    return GridManager;
}

ARTSPlayerController* ARTSVisionManager::ResolveLocalPlayerController() const
{
    return Cast<ARTSPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
}

int32 ARTSVisionManager::ResolveLocalTeamNumber() const
{
    const ARTSPlayerController* PlayerController = ResolveLocalPlayerController();
    const ARTSPlayerState* PlayerState = PlayerController
        ? PlayerController->GetPlayerState<ARTSPlayerState>()
        : nullptr;

    return PlayerState ? PlayerState->TeamNumber : INDEX_NONE;
}

int32 ARTSVisionManager::CoordToVisionIndex(FRTSGridCoord Coord) const
{
    return GridManager ? Coord.Y * GridManager->GridWidth + Coord.X : INDEX_NONE;
}

FRTSGridCoord ARTSVisionManager::VisionIndexToCoord(int32 Index) const
{
    if (!GridManager || GridManager->GridWidth <= 0)
    {
        return FRTSGridCoord();
    }

    return FRTSGridCoord(Index % GridManager->GridWidth, Index / GridManager->GridWidth);
}

bool ARTSVisionManager::IsVisionIndexValid(int32 Index) const
{
    return GridManager
        && Index >= 0
        && Index < GridManager->GridWidth * GridManager->GridHeight;
}

void ARTSVisionManager::ResetCurrentVisibleCells()
{
    for (TPair<int32, FRTSTeamVisionState>& Pair : TeamVisionStates)
    {
        Pair.Value.VisibleCells.Reset();
    }
}

void ARTSVisionManager::MarkActorVision(AActor* Actor, int32 TeamNumber, int32 RadiusCells)
{
    if (!Actor || RadiusCells <= 0 || !GridManager)
    {
        return;
    }

    FRTSTeamVisionState& VisionState = TeamVisionStates.FindOrAdd(TeamNumber);
    MarkVisionCircle(GridManager->WorldToGrid(Actor->GetActorLocation()), RadiusCells, VisionState);
}

void ARTSVisionManager::MarkVisionCircle(FRTSGridCoord CenterCoord, int32 RadiusCells, FRTSTeamVisionState& VisionState) const
{
    if (!GridManager)
    {
        return;
    }

    const int32 ClampedRadius = FMath::Max(0, RadiusCells);
    const int32 RadiusSq = ClampedRadius * ClampedRadius;

    for (int32 Y = -ClampedRadius; Y <= ClampedRadius; ++Y)
    {
        for (int32 X = -ClampedRadius; X <= ClampedRadius; ++X)
        {
            if (X * X + Y * Y > RadiusSq)
            {
                continue;
            }

            const FRTSGridCoord Coord(CenterCoord.X + X, CenterCoord.Y + Y);
            if (!GridManager->IsValidCoord(Coord))
            {
                continue;
            }

            const int32 Index = CoordToVisionIndex(Coord);
            if (!IsVisionIndexValid(Index))
            {
                continue;
            }

            VisionState.VisibleCells.Add(Index);
            VisionState.ExploredCells.Add(Index);
        }
    }
}

int32 ARTSVisionManager::GetUnitSightRadiusCells(const ARTSUnitBase* Unit) const
{
    return Unit && Unit->UnitData
        ? FMath::Max(0, Unit->UnitData->SightRadiusCells)
        : FMath::Max(0, DefaultUnitSightRadiusCells);
}

int32 ARTSVisionManager::GetBuildingSightRadiusCells(const ARTSBuilding* Building) const
{
    return Building && Building->BuildingData
        ? FMath::Max(0, Building->BuildingData->SightRadiusCells)
        : FMath::Max(0, DefaultBuildingSightRadiusCells);
}

int32 ARTSVisionManager::GetActorTeamNumber(AActor* Actor) const
{
    if (const ARTSUnitBase* Unit = Cast<ARTSUnitBase>(Actor))
    {
        return Unit->TeamNumber;
    }

    if (const ARTSBuilding* Building = Cast<ARTSBuilding>(Actor))
    {
        return Building->TeamNumber;
    }

    return INDEX_NONE;
}

bool ARTSVisionManager::ShouldLocallyShowActor(AActor* Actor, int32 ViewerTeamNumber) const
{
    if (!Actor || ViewerTeamNumber < 0)
    {
        return true;
    }

    const int32 ActorTeamNumber = GetActorTeamNumber(Actor);
    if (ActorTeamNumber < 0 || ActorTeamNumber == ViewerTeamNumber)
    {
        return true;
    }

    return IsActorVisibleToTeam(Actor, ViewerTeamNumber);
}

void ARTSVisionManager::UpdateLocalVisionCache()
{
    LocalTeamNumber = ResolveLocalTeamNumber();
    LocalVisibleCells.Reset();
    LocalExploredCells.Reset();

    const FRTSTeamVisionState* VisionState = TeamVisionStates.Find(LocalTeamNumber);
    if (!VisionState)
    {
        return;
    }

    for (const int32 Index : VisionState->VisibleCells)
    {
        LocalVisibleCells.Add(VisionIndexToCoord(Index));
    }

    for (const int32 Index : VisionState->ExploredCells)
    {
        LocalExploredCells.Add(VisionIndexToCoord(Index));
    }
}

void ARTSVisionManager::DrawDebugLocalVision() const
{
    if (!GridManager || !GetWorld())
    {
        return;
    }

    const float HalfCell = GridManager->CellSize * 0.45f;
    const FVector BoxExtent(HalfCell, HalfCell, 4.0f);

    for (const FRTSGridCoord& Coord : LocalVisibleCells)
    {
        FVector Location;
        if (GridManager->GetCellWorldCenterOnGround(Coord, Location))
        {
            DrawDebugBox(GetWorld(), Location + FVector(0.0f, 0.0f, 16.0f), BoxExtent, FColor::Cyan, false, UpdateInterval, 0, 1.0f);
        }
    }
}
