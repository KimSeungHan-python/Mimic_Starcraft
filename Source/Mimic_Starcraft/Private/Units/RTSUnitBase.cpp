// Fill out your copyright notice in the Description page of Project Settings.


#include "Units/RTSUnitBase.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "Core/RTSPlayerState.h"
#include "Data/RTSUnitData.h"

ARTSUnitBase::ARTSUnitBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCanEverAffectNavigation(true);
}

void ARTSUnitBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority() && bCountsTowardSupply && OwningPlayerState && SupplyCost > 0)
    {
        OwningPlayerState->ReleaseSupply(SupplyCost);
        bCountsTowardSupply = false;
    }

    Super::EndPlay(EndPlayReason);
}

void ARTSUnitBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!HasAuthority() || !bHasMoveTarget)
    {
        return;
    }

    const FVector CurrentLocation = GetActorLocation();
    FVector Direction = MoveTargetLocation - CurrentLocation;
    Direction.Z = 0.0f;

    const float Distance = Direction.Size();
    if (Distance <= MovementAcceptanceRadius)
    {
        StopMovement();
        return;
    }

    Direction /= Distance;
    const float Step = FMath::Min(Distance, MovementSpeed * DeltaTime);
    SetActorLocation(CurrentLocation + Direction * Step, true);
    SetActorRotation(Direction.Rotation());
}


void ARTSUnitBase::OnRep_TeamInfo()
{
	ApplyTeamVisual();
}

void ARTSUnitBase::ApplyTeamVisual()
{
	// 여기서 머티리얼 색상 변경, 선택 원 색상 변경 등 처리
}

void ARTSUnitBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARTSUnitBase, TeamNumber);
    DOREPLIFETIME(ARTSUnitBase, TeamColor);
    DOREPLIFETIME(ARTSUnitBase, OwningPlayerState);
    DOREPLIFETIME(ARTSUnitBase, UnitData);
    DOREPLIFETIME(ARTSUnitBase, bHasMoveTarget);
    DOREPLIFETIME(ARTSUnitBase, MoveTargetLocation);
    DOREPLIFETIME(ARTSUnitBase, SupplyCost);
    DOREPLIFETIME(ARTSUnitBase, bCountsTowardSupply);
}

void ARTSUnitBase::IssueMoveCommand(const FVector& TargetLocation)
{
    if (!HasAuthority())
    {
        return;
    }

    MoveTargetLocation = TargetLocation;
    bHasMoveTarget = true;
    SetActorTickEnabled(true);
}

void ARTSUnitBase::StopMovement()
{
    if (!HasAuthority())
    {
        return;
    }

    bHasMoveTarget = false;
    SetActorTickEnabled(false);
}

bool ARTSUnitBase::HasReachedLocation(const FVector& TargetLocation, float AcceptanceRadius) const
{
    FVector Delta = TargetLocation - GetActorLocation();
    Delta.Z = 0.0f;
    return Delta.SizeSquared() <= FMath::Square(AcceptanceRadius);
}

bool ARTSUnitBase::RegisterSupplyCost(int32 InSupplyCost, bool bAlreadyReserved)
{
    if (!HasAuthority())
    {
        return false;
    }

    if (bCountsTowardSupply)
    {
        return true;
    }

    const int32 ClampedSupplyCost = FMath::Max(0, InSupplyCost);
    if (ClampedSupplyCost <= 0)
    {
        SupplyCost = 0;
        bCountsTowardSupply = false;
        return true;
    }

    if (!OwningPlayerState)
    {
        return false;
    }

    if (!bAlreadyReserved && !OwningPlayerState->TryReserveSupply(ClampedSupplyCost))
    {
        return false;
    }

    SupplyCost = ClampedSupplyCost;
    bCountsTowardSupply = true;
    return true;
}

bool ARTSUnitBase::CanReceiveCommandsFrom(AController* Controller) const
{
    const ARTSPlayerState* PlayerState = Controller
        ? Controller->GetPlayerState<ARTSPlayerState>()
        : nullptr;

    return PlayerState && TeamNumber == PlayerState->TeamNumber;
}

bool ARTSUnitBase::CanBeSelectedBy_Implementation(ARTSPlayerController* SelectingController) const
{
    return true;
}

void ARTSUnitBase::SetSelectionState_Implementation(bool bSelected)
{
    bIsSelected = bSelected;
}

bool ARTSUnitBase::IsSelected_Implementation() const
{
    return bIsSelected;
}

int32 ARTSUnitBase::GetSelectableTeamNumber_Implementation() const
{
    return TeamNumber;
}

bool ARTSUnitBase::IsOwnedByPlayerState_Implementation(ARTSPlayerState* PlayerState) const
{
    return PlayerState && TeamNumber == PlayerState->TeamNumber;
}

void ARTSUnitBase::SetTeamInfo(int32 NewTeamNumber, const FLinearColor& NewTeamColor)
{
    if (!HasAuthority())
    {
        return;
    }

    TeamNumber = NewTeamNumber;
    TeamColor = NewTeamColor;

    OnRep_TeamInfo();
}
