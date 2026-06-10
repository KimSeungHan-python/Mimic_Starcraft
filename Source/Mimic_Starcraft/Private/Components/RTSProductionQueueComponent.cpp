#include "Components/RTSProductionQueueComponent.h"

#include "Buildings/RTSBuilding.h"
#include "Core/RTSPlayerState.h"
#include "Data/RTSUnitData.h"
#include "Grid/RTSGridManager.h"
#include "Resources/RTSResourceNode.h"
#include "Units/RTSUnitBase.h"
#include "Units/RTSWorkerUnit.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

URTSProductionQueueComponent::URTSProductionQueueComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
}

void URTSProductionQueueComponent::BeginPlay()
{
    Super::BeginPlay();

    SetComponentTickEnabled(GetOwner() && GetOwner()->HasAuthority());
}

void URTSProductionQueueComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    ProcessProduction();
}

void URTSProductionQueueComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(URTSProductionQueueComponent, ProductionQueue);
    DOREPLIFETIME(URTSProductionQueueComponent, bUseRallyPoint);
    DOREPLIFETIME(URTSProductionQueueComponent, RallyPointWorldLocation);
    DOREPLIFETIME(URTSProductionQueueComponent, RallyResourceTarget);
}

bool URTSProductionQueueComponent::CanQueueUnit(URTSUnitData* UnitData, ARTSPlayerState* PlayerState) const
{
    const ARTSBuilding* OwningBuilding = GetOwningBuilding();
    const ARTSPlayerState* EffectivePlayerState = PlayerState ? PlayerState : ResolvePlayerState(nullptr);

    return OwningBuilding
        && OwningBuilding->BuildingState == ERTSBuildingState::Completed
        && UnitData
        && UnitData->UnitClass
        && ProductionQueue.Num() < MaxQueueSize
        && EffectivePlayerState
        && EffectivePlayerState->CanAfford(UnitData->MineralCost, UnitData->VespeneCost)
        && EffectivePlayerState->CanReserveSupply(UnitData->SupplyCost);
}

bool URTSProductionQueueComponent::QueueUnit(URTSUnitData* UnitData, ARTSPlayerState* PlayerState)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return false;
    }

    ARTSPlayerState* EffectivePlayerState = ResolvePlayerState(PlayerState);
    if (!CanQueueUnit(UnitData, EffectivePlayerState))
    {
        return false;
    }

    if (!EffectivePlayerState->TrySpendResources(UnitData->MineralCost, UnitData->VespeneCost))
    {
        return false;
    }

    if (!EffectivePlayerState->TryReserveSupply(UnitData->SupplyCost))
    {
        EffectivePlayerState->AddResources(UnitData->MineralCost, UnitData->VespeneCost);
        return false;
    }

    FRTSProductionQueueItem NewItem;
    NewItem.UnitData = UnitData;
    NewItem.Duration = FMath::Max(0.01f, UnitData->BuildTime);

    if (ProductionQueue.Num() == 0)
    {
        NewItem.StartServerTime = GetServerTimeSeconds();
    }

    ProductionQueue.Add(NewItem);
    SetComponentTickEnabled(true);
    BroadcastQueueChanged();

    return true;
}

bool URTSProductionQueueComponent::CancelQueuedUnit(int32 QueueIndex, bool bRefundResources)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return false;
    }

    if (!ProductionQueue.IsValidIndex(QueueIndex))
    {
        return false;
    }

    URTSUnitData* UnitData = ProductionQueue[QueueIndex].UnitData;
    ARTSPlayerState* PlayerState = ResolvePlayerState(nullptr);

    if (bRefundResources && UnitData && PlayerState)
    {
        PlayerState->AddResources(UnitData->MineralCost, UnitData->VespeneCost);
    }

    if (UnitData && PlayerState)
    {
        PlayerState->ReleaseSupply(UnitData->SupplyCost);
    }

    ProductionQueue.RemoveAt(QueueIndex);
    StartNextQueueItemIfNeeded();
    SetComponentTickEnabled(ProductionQueue.Num() > 0);
    BroadcastQueueChanged();

    return true;
}

bool URTSProductionQueueComponent::HasQueuedProduction() const
{
    return ProductionQueue.Num() > 0;
}

float URTSProductionQueueComponent::GetCurrentProductionProgress01() const
{
    if (ProductionQueue.Num() == 0)
    {
        return 0.0f;
    }

    const FRTSProductionQueueItem& CurrentItem = ProductionQueue[0];
    if (CurrentItem.Duration <= KINDA_SMALL_NUMBER)
    {
        return 1.0f;
    }

    const float Elapsed = GetServerTimeSeconds() - CurrentItem.StartServerTime;
    return FMath::Clamp(Elapsed / CurrentItem.Duration, 0.0f, 1.0f);
}

FTransform URTSProductionQueueComponent::GetSpawnTransform() const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return FTransform::Identity;
    }

    FVector SpawnLocation = GetDesiredSpawnLocation();
    FVector AdjustedSpawnLocation;
    if (FindNearestWalkableSpawnLocation(SpawnLocation, AdjustedSpawnLocation))
    {
        SpawnLocation = AdjustedSpawnLocation;
    }

    const FRotator SpawnRotation = Owner->GetActorRotation();

    return FTransform(SpawnRotation, SpawnLocation);
}

void URTSProductionQueueComponent::SetRallyPoint(const FVector& WorldLocation)
{
    SetRallyPointTarget(WorldLocation, nullptr);
}

void URTSProductionQueueComponent::SetRallyPointTarget(const FVector& WorldLocation, ARTSResourceNode* ResourceTarget)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    RallyResourceTarget = ResourceTarget && ResourceTarget->HasResources()
        ? ResourceTarget
        : nullptr;
    RallyPointWorldLocation = RallyResourceTarget
        ? RallyResourceTarget->GetGatherLocation()
        : WorldLocation;
    bUseRallyPoint = true;
}

void URTSProductionQueueComponent::ClearRallyPoint()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    RallyPointWorldLocation = FVector::ZeroVector;
    RallyResourceTarget = nullptr;
    bUseRallyPoint = false;
}

bool URTSProductionQueueComponent::HasRallyPoint() const
{
    return bUseRallyPoint;
}

void URTSProductionQueueComponent::OnRep_ProductionQueue()
{
    BroadcastQueueChanged();
}

ARTSBuilding* URTSProductionQueueComponent::GetOwningBuilding() const
{
    return Cast<ARTSBuilding>(GetOwner());
}

ARTSPlayerState* URTSProductionQueueComponent::ResolvePlayerState(ARTSPlayerState* ExplicitPlayerState) const
{
    if (ExplicitPlayerState)
    {
        return ExplicitPlayerState;
    }

    const ARTSBuilding* OwningBuilding = GetOwningBuilding();
    return OwningBuilding ? OwningBuilding->OwningPlayerState : nullptr;
}

float URTSProductionQueueComponent::GetServerTimeSeconds() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return 0.0f;
    }

    const AGameStateBase* GameState = World->GetGameState<AGameStateBase>();
    if (GameState)
    {
        return GameState->GetServerWorldTimeSeconds();
    }

    return World->GetTimeSeconds();
}

FVector URTSProductionQueueComponent::GetDesiredSpawnLocation() const
{
    const AActor* Owner = GetOwner();
    return Owner
        ? Owner->GetActorTransform().TransformPosition(SpawnPointLocalOffset)
        : FVector::ZeroVector;
}

bool URTSProductionQueueComponent::FindNearestWalkableSpawnLocation(
    const FVector& DesiredWorldLocation,
    FVector& OutLocation
) const
{
    const ARTSBuilding* OwningBuilding = GetOwningBuilding();
    ARTSGridManager* GridManager = OwningBuilding ? OwningBuilding->OwningGridManager.Get() : nullptr;

    if (!GridManager)
    {
        return false;
    }

    const FRTSGridCoord DesiredCoord = GridManager->WorldToGrid(DesiredWorldLocation);
    float BestDistSq = TNumericLimits<float>::Max();
    bool bFoundLocation = false;

    for (int32 Radius = 0; Radius <= SpawnSearchRadiusCells; ++Radius)
    {
        for (int32 Y = -Radius; Y <= Radius; ++Y)
        {
            for (int32 X = -Radius; X <= Radius; ++X)
            {
                if (Radius > 0 && FMath::Max(FMath::Abs(X), FMath::Abs(Y)) != Radius)
                {
                    continue;
                }

                const FRTSGridCoord CandidateCoord(DesiredCoord.X + X, DesiredCoord.Y + Y);
                if (!GridManager->IsCellWalkable(CandidateCoord))
                {
                    continue;
                }

                FVector CandidateLocation;
                if (!GridManager->GetCellWorldCenterOnGround(CandidateCoord, CandidateLocation))
                {
                    continue;
                }

                const float DistSq = FVector::DistSquared2D(DesiredWorldLocation, CandidateLocation);
                if (DistSq < BestDistSq)
                {
                    BestDistSq = DistSq;
                    OutLocation = CandidateLocation;
                    bFoundLocation = true;
                }
            }
        }

        if (bFoundLocation)
        {
            return true;
        }
    }

    return false;
}

void URTSProductionQueueComponent::ApplyProductionRally(ARTSUnitBase* NewUnit) const
{
    if (!NewUnit || !bUseRallyPoint)
    {
        return;
    }

    if (RallyResourceTarget && RallyResourceTarget->HasResources())
    {
        if (ARTSWorkerUnit* Worker = Cast<ARTSWorkerUnit>(NewUnit))
        {
            if (Worker->GatherFromResource(RallyResourceTarget))
            {
                return;
            }
        }

        NewUnit->IssueMoveCommand(RallyResourceTarget->GetGatherLocation());
        return;
    }

    NewUnit->IssueMoveCommand(RallyPointWorldLocation);
}

void URTSProductionQueueComponent::ProcessProduction()
{
    if (ProductionQueue.Num() == 0)
    {
        SetComponentTickEnabled(false);
        return;
    }

    StartNextQueueItemIfNeeded();

    if (GetCurrentProductionProgress01() >= 1.0f)
    {
        FinishCurrentProduction();
    }
}

void URTSProductionQueueComponent::FinishCurrentProduction()
{
    if (ProductionQueue.Num() == 0)
    {
        return;
    }

    URTSUnitData* UnitData = ProductionQueue[0].UnitData;
    ARTSBuilding* OwningBuilding = GetOwningBuilding();
    UWorld* World = GetWorld();

    if (UnitData && UnitData->UnitClass && OwningBuilding && World)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = OwningBuilding->GetOwner();
        SpawnParams.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ARTSUnitBase* NewUnit = World->SpawnActor<ARTSUnitBase>(
            UnitData->UnitClass,
            GetSpawnTransform(),
            SpawnParams
        );

        if (NewUnit)
        {
            NewUnit->SetUnitData(UnitData);
            NewUnit->OwningPlayerState = OwningBuilding->OwningPlayerState;
            NewUnit->SetTeamInfo(OwningBuilding->TeamNumber, OwningBuilding->TeamColor);
            NewUnit->RegisterSupplyCost(UnitData->SupplyCost, true);

            ApplyProductionRally(NewUnit);
        }
        else if (OwningBuilding->OwningPlayerState)
        {
            OwningBuilding->OwningPlayerState->ReleaseSupply(UnitData->SupplyCost);
        }
    }

    ProductionQueue.RemoveAt(0);
    StartNextQueueItemIfNeeded();
    SetComponentTickEnabled(ProductionQueue.Num() > 0);
    BroadcastQueueChanged();
}

void URTSProductionQueueComponent::StartNextQueueItemIfNeeded()
{
    if (ProductionQueue.Num() == 0)
    {
        return;
    }

    FRTSProductionQueueItem& CurrentItem = ProductionQueue[0];
    if (CurrentItem.StartServerTime <= 0.0f)
    {
        CurrentItem.StartServerTime = GetServerTimeSeconds();
    }
}

void URTSProductionQueueComponent::BroadcastQueueChanged()
{
    OnProductionQueueChanged.Broadcast();
}
