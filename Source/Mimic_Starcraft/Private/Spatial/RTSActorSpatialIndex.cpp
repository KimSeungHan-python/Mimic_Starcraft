#include "Spatial/RTSActorSpatialIndex.h"

#include "EngineUtils.h"
#include "Grid/RTSGridManager.h"
#include "UObject/ObjectKey.h"

ARTSActorSpatialIndex::ARTSActorSpatialIndex()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.TickInterval = UpdateInterval;
    bReplicates = false;
}

void ARTSActorSpatialIndex::BeginPlay()
{
    Super::BeginPlay();

    ResolveGridManager();
    PrimaryActorTick.TickInterval = FMath::Max(0.01f, UpdateInterval);
}

void ARTSActorSpatialIndex::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TArray<TObjectKey<AActor>> KeysToRemove;

    for (TPair<TObjectKey<AActor>, FRTSSpatialActorEntry>& Pair : ActorEntries)
    {
        if (!Pair.Value.Actor.IsValid())
        {
            KeysToRemove.Add(Pair.Key);
            continue;
        }

        UpdateActorCell(Pair.Key, Pair.Value);
    }

    for (const TObjectKey<AActor>& ActorKey : KeysToRemove)
    {
        if (FRTSSpatialActorEntry* Entry = ActorEntries.Find(ActorKey))
        {
            RemoveFromBucket(ActorKey, Entry->Cell);
        }

        ActorEntries.Remove(ActorKey);
    }
}

void ARTSActorSpatialIndex::RegisterActor(AActor* Actor)
{
    if (!Actor)
    {
        return;
    }

    const TObjectKey<AActor> ActorKey(Actor);
    if (ActorEntries.Contains(ActorKey))
    {
        return;
    }

    FRTSSpatialActorEntry Entry;
    Entry.Actor = Actor;
    Entry.Cell = WorldToSpatialCell(Actor->GetActorLocation());

    ActorEntries.Add(ActorKey, Entry);
    AddToBucket(ActorKey, Entry.Cell);
}

void ARTSActorSpatialIndex::UnregisterActor(AActor* Actor)
{
    if (!Actor)
    {
        return;
    }

    const TObjectKey<AActor> ActorKey(Actor);
    if (FRTSSpatialActorEntry* Entry = ActorEntries.Find(ActorKey))
    {
        RemoveFromBucket(ActorKey, Entry->Cell);
    }

    ActorEntries.Remove(ActorKey);
}

void ARTSActorSpatialIndex::QueryActorsInRadius(
    const FVector& WorldLocation,
    float Radius,
    TArray<AActor*>& OutActors
) const
{
    OutActors.Reset();

    const float ClampedRadius = FMath::Max(0.0f, Radius);
    const float RadiusSq = FMath::Square(ClampedRadius);
    const float CellSize = GetEffectiveCellSize();
    const int32 CellRadius = FMath::CeilToInt(ClampedRadius / CellSize);
    const FRTSGridCoord CenterCell = WorldToSpatialCell(WorldLocation);

    TSet<TObjectKey<AActor>> SeenActors;

    for (int32 Y = -CellRadius; Y <= CellRadius; ++Y)
    {
        for (int32 X = -CellRadius; X <= CellRadius; ++X)
        {
            const FRTSGridCoord Cell(CenterCell.X + X, CenterCell.Y + Y);
            const TArray<TObjectKey<AActor>>* Bucket = Buckets.Find(Cell);
            if (!Bucket)
            {
                continue;
            }

            for (const TObjectKey<AActor>& ActorKey : *Bucket)
            {
                if (SeenActors.Contains(ActorKey))
                {
                    continue;
                }

                AActor* Actor = ActorKey.ResolveObjectPtr();
                if (!Actor)
                {
                    continue;
                }

                if (FVector::DistSquared2D(WorldLocation, Actor->GetActorLocation()) > RadiusSq)
                {
                    continue;
                }

                SeenActors.Add(ActorKey);
                OutActors.Add(Actor);
            }
        }
    }
}

ARTSActorSpatialIndex* ARTSActorSpatialIndex::FindExisting(UWorld* World)
{
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<ARTSActorSpatialIndex> It(World); It; ++It)
    {
        return *It;
    }

    return nullptr;
}

ARTSActorSpatialIndex* ARTSActorSpatialIndex::GetOrCreate(UWorld* World)
{
    if (ARTSActorSpatialIndex* ExistingIndex = FindExisting(World))
    {
        return ExistingIndex;
    }

    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    return World->SpawnActor<ARTSActorSpatialIndex>(
        ARTSActorSpatialIndex::StaticClass(),
        FTransform::Identity,
        SpawnParams
    );
}

FRTSGridCoord ARTSActorSpatialIndex::WorldToSpatialCell(const FVector& WorldLocation) const
{
    if (GridManager)
    {
        return GridManager->WorldToGrid(WorldLocation);
    }

    const float CellSize = GetEffectiveCellSize();
    return FRTSGridCoord(
        FMath::FloorToInt(WorldLocation.X / CellSize),
        FMath::FloorToInt(WorldLocation.Y / CellSize)
    );
}

float ARTSActorSpatialIndex::GetEffectiveCellSize() const
{
    return GridManager
        ? FMath::Max(1.0f, GridManager->CellSize)
        : FMath::Max(1.0f, SpatialCellSize);
}

void ARTSActorSpatialIndex::ResolveGridManager()
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

void ARTSActorSpatialIndex::AddToBucket(const TObjectKey<AActor>& ActorKey, FRTSGridCoord Cell)
{
    Buckets.FindOrAdd(Cell).AddUnique(ActorKey);
}

void ARTSActorSpatialIndex::RemoveFromBucket(const TObjectKey<AActor>& ActorKey, FRTSGridCoord Cell)
{
    TArray<TObjectKey<AActor>>* Bucket = Buckets.Find(Cell);
    if (!Bucket)
    {
        return;
    }

    Bucket->Remove(ActorKey);
    if (Bucket->Num() == 0)
    {
        Buckets.Remove(Cell);
    }
}

void ARTSActorSpatialIndex::UpdateActorCell(const TObjectKey<AActor>& ActorKey, FRTSSpatialActorEntry& Entry)
{
    AActor* Actor = Entry.Actor.Get();
    if (!Actor)
    {
        return;
    }

    const FRTSGridCoord NewCell = WorldToSpatialCell(Actor->GetActorLocation());
    if (NewCell == Entry.Cell)
    {
        return;
    }

    RemoveFromBucket(ActorKey, Entry.Cell);
    Entry.Cell = NewCell;
    AddToBucket(ActorKey, Entry.Cell);
}
