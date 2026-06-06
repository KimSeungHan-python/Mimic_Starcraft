#include "Resources/RTSResourceNode.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Grid/RTSGridManager.h"
#include "Net/UnrealNetwork.h"

ARTSResourceNode::ARTSResourceNode()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(SceneRoot);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
}

void ARTSResourceNode::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        RemainingAmount = FMath::Clamp(RemainingAmount, 0, FMath::Max(0, MaxAmount));
    }
}

void ARTSResourceNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARTSResourceNode, RemainingAmount);
}

bool ARTSResourceNode::HasResources() const
{
    return RemainingAmount > 0;
}

int32 ARTSResourceNode::Harvest(int32 RequestedAmount)
{
    if (!HasAuthority() || RequestedAmount <= 0 || RemainingAmount <= 0)
    {
        return 0;
    }

    const int32 HarvestedAmount = FMath::Min(RequestedAmount, RemainingAmount);
    RemainingAmount -= HarvestedAmount;
    OnRep_RemainingAmount();

    return HarvestedAmount;
}

FVector ARTSResourceNode::GetGatherLocation() const
{
    return GetActorTransform().TransformPosition(GatherPointLocalOffset);
}

FRTSGridCoord ARTSResourceNode::GetGridOriginCoord(ARTSGridManager* GridManager) const
{
    if (!GridManager)
    {
        return FRTSGridCoord();
    }

    const FRTSGridCoord CenterCoord = GridManager->WorldToGrid(GetActorLocation());
    return FRTSGridCoord(
        CenterCoord.X - FMath::Max(1, GridWidth) / 2,
        CenterCoord.Y - FMath::Max(1, GridHeight) / 2
    );
}

void ARTSResourceNode::OnRep_RemainingAmount()
{
}
