#include "Units/RTSWorkerUnit.h"

#include "Components/RTSAttackComponent.h"
#include "Components/RTSGatherComponent.h"
#include "Components/RTSWorkerBuildComponent.h"

ARTSWorkerUnit::ARTSWorkerUnit()
{
    GatherComponent = CreateDefaultSubobject<URTSGatherComponent>(TEXT("GatherComponent"));
    BuildComponent = CreateDefaultSubobject<URTSWorkerBuildComponent>(TEXT("BuildComponent"));
}

bool ARTSWorkerUnit::GatherFromResource(ARTSResourceNode* ResourceNode)
{
    //서버이에서 실행하고 자원 채집 상태일때는 다른 컴포넌트들 행동 중지
    if (HasAuthority() && BuildComponent)
    {
        BuildComponent->StopBuildOrder();
    }

    if (HasAuthority() && AttackComponent)
    {
        AttackComponent->StopAttackCommand();
    }

    return HasAuthority() && GatherComponent && GatherComponent->StartGathering(ResourceNode);
}

void ARTSWorkerUnit::StopAllCommands()
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

    if (AttackComponent)
    {
        AttackComponent->StopAttackCommand();
    }

    StopMovement();
}

void ARTSWorkerUnit::StopWorkerCommand()
{
    StopAllCommands();
}
