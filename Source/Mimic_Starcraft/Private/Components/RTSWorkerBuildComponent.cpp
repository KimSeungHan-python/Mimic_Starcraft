#include "Components/RTSWorkerBuildComponent.h"

#include "Buildings/RTSBuilding.h"
#include "Components/RTSAttackComponent.h"
#include "Components/RTSGatherComponent.h"
#include "Core/RTSPlayerController.h"
#include "Data/RTSBuildingData.h"
#include "Grid/RTSGridManager.h"
#include "Units/RTSWorkerUnit.h"

#include "Net/UnrealNetwork.h"

URTSWorkerBuildComponent::URTSWorkerBuildComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
}

void URTSWorkerBuildComponent::BeginPlay()
{
    Super::BeginPlay();

    SetComponentTickEnabled(GetOwner() && GetOwner()->HasAuthority() && BuildState != ERTSWorkerBuildState::Idle);
}

void URTSWorkerBuildComponent::TickComponent(
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

    if (BuildState == ERTSWorkerBuildState::MovingToBuildSite)
    {
        TryCompleteBuildOrder();
    }
    else
    {
        SetComponentTickEnabled(false);
    }
}

void URTSWorkerBuildComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(URTSWorkerBuildComponent, BuildState);
    DOREPLIFETIME(URTSWorkerBuildComponent, TargetBuildingData);
    DOREPLIFETIME(URTSWorkerBuildComponent, TargetOriginCoord);
    DOREPLIFETIME(URTSWorkerBuildComponent, TargetBuildLocation);
}

bool URTSWorkerBuildComponent::StartBuildOrder(
    ARTSPlayerController* BuildController,
    URTSBuildingData* BuildingData,
    FRTSGridCoord OriginCoord,
    ARTSGridManager* GridManager
)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !BuildController || !BuildingData || !GridManager)
    {
        return false;
    }

    ARTSWorkerUnit* Worker = GetOwningWorker();
    if (!Worker || !Worker->CanReceiveCommandsFrom(BuildController))
    {
        return false;
    }

    FVector BuildLocation;
    if (!GridManager->GetBuildingCenterLocationOnGround(
        OriginCoord,
        FMath::Max(1, BuildingData->GridWidth),
        FMath::Max(1, BuildingData->GridHeight),
        BuildLocation
    ))
    {
        return false;
    }

    if (Worker->GatherComponent)
    {
        Worker->GatherComponent->StopGathering();
    }

    Worker->StopMovement();
    if (Worker->AttackComponent)
    {
        Worker->AttackComponent->StopAttackCommand();
    }

    OwningBuildController = BuildController;
    TargetBuildingData = BuildingData;
    TargetOriginCoord = OriginCoord;
    TargetBuildLocation = BuildLocation;

    Worker->IssueMoveCommand(TargetBuildLocation);
    SetBuildState(ERTSWorkerBuildState::MovingToBuildSite);
    return true;
}

void URTSWorkerBuildComponent::StopBuildOrder()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    OwningBuildController = nullptr;
    TargetBuildingData = nullptr;
    TargetOriginCoord = FRTSGridCoord();
    TargetBuildLocation = FVector::ZeroVector;
    SetBuildState(ERTSWorkerBuildState::Idle);
}

bool URTSWorkerBuildComponent::HasActiveBuildOrder() const
{
    return BuildState != ERTSWorkerBuildState::Idle;
}

void URTSWorkerBuildComponent::OnRep_BuildState()
{
}

ARTSWorkerUnit* URTSWorkerBuildComponent::GetOwningWorker() const
{
    return Cast<ARTSWorkerUnit>(GetOwner());
}

void URTSWorkerBuildComponent::SetBuildState(ERTSWorkerBuildState NewState)
{
    BuildState = NewState;
    SetComponentTickEnabled(NewState != ERTSWorkerBuildState::Idle);
    OnRep_BuildState();
}

bool URTSWorkerBuildComponent::IsAtBuildSite() const
{
    const AActor* OwnerActor = GetOwner();
    return OwnerActor
        && FVector::DistSquared(OwnerActor->GetActorLocation(), TargetBuildLocation)
            <= FMath::Square(BuildStartAcceptanceRadius);
}

void URTSWorkerBuildComponent::TryCompleteBuildOrder()
{
    if (!TargetBuildingData || !OwningBuildController)
    {
        StopBuildOrder();
        return;
    }

    if (!IsAtBuildSite())
    {
        return;
    }

    if (ARTSWorkerUnit* Worker = GetOwningWorker())
    {
        Worker->StopMovement();
        const bool bBuilt = OwningBuildController->CompleteWorkerBuildOrder(
            Worker,
            TargetBuildingData->BuildingId,
            TargetOriginCoord
        );

        StopBuildOrder();

        if (!bBuilt)
        {
            Worker->StopMovement();
        }
    }
}
