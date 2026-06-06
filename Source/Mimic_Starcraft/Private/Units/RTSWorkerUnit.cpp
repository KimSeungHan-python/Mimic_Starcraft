#include "Units/RTSWorkerUnit.h"

#include "Components/RTSGatherComponent.h"

ARTSWorkerUnit::ARTSWorkerUnit()
{
    GatherComponent = CreateDefaultSubobject<URTSGatherComponent>(TEXT("GatherComponent"));
}

bool ARTSWorkerUnit::GatherFromResource(ARTSResourceNode* ResourceNode)
{
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

    StopMovement();
}
