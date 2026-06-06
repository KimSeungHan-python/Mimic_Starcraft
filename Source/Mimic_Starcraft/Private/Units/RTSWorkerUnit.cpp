#include "Units/RTSWorkerUnit.h"

#include "Components/RTSGatherComponent.h"
#include "Components/RTSWorkerBuildComponent.h"

ARTSWorkerUnit::ARTSWorkerUnit()
{
    GatherComponent = CreateDefaultSubobject<URTSGatherComponent>(TEXT("GatherComponent"));
    BuildComponent = CreateDefaultSubobject<URTSWorkerBuildComponent>(TEXT("BuildComponent"));
}

bool ARTSWorkerUnit::GatherFromResource(ARTSResourceNode* ResourceNode)
{
    if (HasAuthority() && BuildComponent)
    {
        BuildComponent->StopBuildOrder();
    }

    return HasAuthority() && GatherComponent && GatherComponent->StartGathering(ResourceNode);
}

void ARTSWorkerUnit::StopWorkerCommand()
{
    if (!HasAuthority())
    {
        return;
    }

    if (GatherComponent)
    {
        GatherComponent->StopGathering();
    }

    if (BuildComponent)
    {
        BuildComponent->StopBuildOrder();
    }

    StopMovement();
}
