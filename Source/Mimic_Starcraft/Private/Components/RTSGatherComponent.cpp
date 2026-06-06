#include "Components/RTSGatherComponent.h"

#include "Buildings/RTSBuilding.h"
#include "Core/RTSPlayerState.h"
#include "Data/RTSBuildingData.h"
#include "Units/RTSUnitBase.h"

#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

URTSGatherComponent::URTSGatherComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
}

void URTSGatherComponent::BeginPlay()
{
    Super::BeginPlay();

    SetComponentTickEnabled(GetOwner() && GetOwner()->HasAuthority() && GatherState != ERTSGatherState::Idle);
}

void URTSGatherComponent::TickComponent(
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

    switch (GatherState)
    {
    case ERTSGatherState::MovingToResource:
        ProcessMovingToResource();
        break;

    case ERTSGatherState::Gathering:
        ProcessGathering(DeltaTime);
        break;

    case ERTSGatherState::ReturningToDropOff:
        ProcessReturningToDropOff();
        break;

    default:
        SetComponentTickEnabled(false);
        break;
    }
}

void URTSGatherComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(URTSGatherComponent, GatherState);
    DOREPLIFETIME(URTSGatherComponent, TargetResourceNode);
}

bool URTSGatherComponent::StartGathering(ARTSResourceNode* ResourceNode)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !ResourceNode || !ResourceNode->HasResources())
    {
        return false;
    }

    TargetResourceNode = ResourceNode;
    CurrentDropOffBuilding = FindNearestDropOff(ResourceNode->ResourceType);

    if (!CurrentDropOffBuilding)
    {
        return false;
    }

    MoveTowardResource();
    return true;
}

void URTSGatherComponent::StopGathering()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    TargetResourceNode = nullptr;
    CurrentDropOffBuilding = nullptr;
    HarvestTimer = 0.0f;
    CarriedMinerals = 0;
    CarriedVespene = 0;

    if (ARTSUnitBase* Unit = GetOwningUnit())
    {
        Unit->StopMovement();
    }

    SetGatherState(ERTSGatherState::Idle);
}

bool URTSGatherComponent::IsCarryingResources() const
{
    return CarriedMinerals > 0 || CarriedVespene > 0;
}

void URTSGatherComponent::OnRep_GatherState()
{
}

ARTSUnitBase* URTSGatherComponent::GetOwningUnit() const
{
    return Cast<ARTSUnitBase>(GetOwner());
}

ARTSPlayerState* URTSGatherComponent::GetOwningPlayerState() const
{
    const ARTSUnitBase* Unit = GetOwningUnit();
    return Unit ? Unit->OwningPlayerState : nullptr;
}

ARTSBuilding* URTSGatherComponent::FindNearestDropOff(ERTSResourceType ResourceType) const
{
    const ARTSPlayerState* PlayerState = GetOwningPlayerState();
    const AActor* OwnerActor = GetOwner();
    UWorld* World = GetWorld();

    if (!PlayerState || !OwnerActor || !World)
    {
        return nullptr;
    }

    ARTSBuilding* BestBuilding = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    for (TActorIterator<ARTSBuilding> It(World); It; ++It)
    {
        ARTSBuilding* Building = *It;
        if (!Building || !Building->BuildingData || Building->TeamNumber != PlayerState->TeamNumber)
        {
            continue;
        }

        if (Building->BuildingState != ERTSBuildingState::Completed)
        {
            continue;
        }

        const bool bAcceptsResource = ResourceType == ERTSResourceType::Minerals
            ? Building->BuildingData->bAcceptsMineralDropOff
            : Building->BuildingData->bAcceptsVespeneDropOff;

        if (!bAcceptsResource)
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(OwnerActor->GetActorLocation(), Building->GetActorLocation());
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestBuilding = Building;
        }
    }

    return BestBuilding;
}

bool URTSGatherComponent::IsNearLocation(const FVector& Location) const
{
    const AActor* OwnerActor = GetOwner();
    return OwnerActor
        && FVector::DistSquared(OwnerActor->GetActorLocation(), Location) <= FMath::Square(InteractionRadius);
}

void URTSGatherComponent::SetGatherState(ERTSGatherState NewState)
{
    GatherState = NewState;
    SetComponentTickEnabled(NewState != ERTSGatherState::Idle);
    OnRep_GatherState();
}

void URTSGatherComponent::MoveTowardResource()
{
    ARTSUnitBase* Unit = GetOwningUnit();
    if (!Unit || !TargetResourceNode)
    {
        StopGathering();
        return;
    }

    Unit->IssueMoveCommand(TargetResourceNode->GetGatherLocation());
    SetGatherState(ERTSGatherState::MovingToResource);
}

void URTSGatherComponent::MoveTowardDropOff()
{
    ARTSUnitBase* Unit = GetOwningUnit();
    if (!Unit || !CurrentDropOffBuilding)
    {
        StopGathering();
        return;
    }

    Unit->IssueMoveCommand(CurrentDropOffBuilding->GetActorLocation());
    SetGatherState(ERTSGatherState::ReturningToDropOff);
}

void URTSGatherComponent::ProcessMovingToResource()
{
    if (!TargetResourceNode || !TargetResourceNode->HasResources())
    {
        StopGathering();
        return;
    }

    if (!IsNearLocation(TargetResourceNode->GetGatherLocation()))
    {
        return;
    }

    if (ARTSUnitBase* Unit = GetOwningUnit())
    {
        Unit->StopMovement();
    }

    HarvestTimer = HarvestDuration;
    SetGatherState(ERTSGatherState::Gathering);
}

void URTSGatherComponent::ProcessGathering(float DeltaTime)
{
    if (!TargetResourceNode || !TargetResourceNode->HasResources())
    {
        StopGathering();
        return;
    }

    HarvestTimer -= DeltaTime;
    if (HarvestTimer > 0.0f)
    {
        return;
    }

    const int32 HarvestedAmount = TargetResourceNode->Harvest(CarryCapacity);
    if (HarvestedAmount <= 0)
    {
        StopGathering();
        return;
    }

    if (TargetResourceNode->ResourceType == ERTSResourceType::Minerals)
    {
        CarriedMinerals = HarvestedAmount;
        CarriedVespene = 0;
    }
    else
    {
        CarriedMinerals = 0;
        CarriedVespene = HarvestedAmount;
    }

    if (!CurrentDropOffBuilding)
    {
        CurrentDropOffBuilding = FindNearestDropOff(TargetResourceNode->ResourceType);
    }

    if (!CurrentDropOffBuilding)
    {
        StopGathering();
        return;
    }

    MoveTowardDropOff();
}

void URTSGatherComponent::ProcessReturningToDropOff()
{
    if (!CurrentDropOffBuilding)
    {
        StopGathering();
        return;
    }

    if (!IsNearLocation(CurrentDropOffBuilding->GetActorLocation()))
    {
        return;
    }

    if (ARTSUnitBase* Unit = GetOwningUnit())
    {
        Unit->StopMovement();
    }

    DepositCarriedResources();

    if (TargetResourceNode && TargetResourceNode->HasResources())
    {
        MoveTowardResource();
        return;
    }

    StopGathering();
}

void URTSGatherComponent::DepositCarriedResources()
{
    ARTSPlayerState* PlayerState = GetOwningPlayerState();
    if (PlayerState && IsCarryingResources())
    {
        PlayerState->AddResources(CarriedMinerals, CarriedVespene);
    }

    CarriedMinerals = 0;
    CarriedVespene = 0;
}
