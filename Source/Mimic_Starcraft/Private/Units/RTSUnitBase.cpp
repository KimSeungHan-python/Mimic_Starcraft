// Fill out your copyright notice in the Description page of Project Settings.


#include "Units/RTSUnitBase.h"
#include "Components/MeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/RTSAttackComponent.h"
#include "Components/RTSCombatEffectsComponent.h"
#include "Components/RTSHealthComponent.h"
#include "Components/RTSUnitMovementComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Core/RTSPlayerState.h"
#include "Data/RTSUnitData.h"
#include "Spatial/RTSActorSpatialIndex.h"

ARTSUnitBase::ARTSUnitBase()
{
	bReplicates = true;
	SetReplicateMovement(true);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCanEverAffectNavigation(true);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComponent->SetupAttachment(SceneRoot);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	SkeletalMeshComponent->SetCanEverAffectNavigation(false);
	SkeletalMeshComponent->SetHiddenInGame(true);
	SkeletalMeshComponent->SetVisibility(false);

	SelectionDecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("SelectionDecalComponent"));
	SelectionDecalComponent->SetupAttachment(SceneRoot);
	SelectionDecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	SelectionDecalComponent->SetRelativeLocation(FVector(0.0f, 0.0f, SelectionDecalZOffset));
	SelectionDecalComponent->DecalSize = SelectionDecalSize;
	SelectionDecalComponent->SetHiddenInGame(true);
	SelectionDecalComponent->SetVisibility(false);
	SelectionDecalComponent->SetFadeScreenSize(0.0f);

	HealthComponent = CreateDefaultSubobject<URTSHealthComponent>(TEXT("HealthComponent"));
	AttackComponent = CreateDefaultSubobject<URTSAttackComponent>(TEXT("AttackComponent"));
	CombatEffectsComponent = CreateDefaultSubobject<URTSCombatEffectsComponent>(TEXT("CombatEffectsComponent"));
	UnitMovementComponent = CreateDefaultSubobject<URTSUnitMovementComponent>(TEXT("UnitMovementComponent"));

}

void ARTSUnitBase::BeginPlay()
{
	Super::BeginPlay();

	RefreshUnitVisual();

	if (HasAuthority())
	{
		if (ARTSActorSpatialIndex* SpatialIndex = ARTSActorSpatialIndex::GetOrCreate(GetWorld()))
		{
			SpatialIndex->RegisterActor(this);
		}
	}
}

void ARTSUnitBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority())
    {
        if (ARTSActorSpatialIndex* SpatialIndex = ARTSActorSpatialIndex::FindExisting(GetWorld()))
        {
            SpatialIndex->UnregisterActor(this);
        }
    }

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

    if (UnitMovementComponent)
    {
        return;
    }

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

void ARTSUnitBase::OnRep_UnitData()
{
	RefreshUnitVisual();
}

void ARTSUnitBase::ApplyTeamVisual()
{
	if (!bApplyTeamColorToMaterials)
	{
		return;
	}

	ApplyTeamVisualToMesh(MeshComponent);
	ApplyTeamVisualToMesh(SkeletalMeshComponent);
}

void ARTSUnitBase::ApplyTeamVisualToMesh(UMeshComponent* TargetMesh)
{
	if (!TargetMesh || !TargetMesh->IsVisible())
	{
		return;
	}

	const int32 MaterialCount = TargetMesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		if (!TargetMesh->GetMaterial(MaterialIndex))
		{
			continue;
		}

		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(TargetMesh->GetMaterial(MaterialIndex));
		if (!DynamicMaterial)
		{
			DynamicMaterial = TargetMesh->CreateDynamicMaterialInstance(MaterialIndex);
		}

		if (!DynamicMaterial)
		{
			continue;
		}

		DynamicMaterial->SetVectorParameterValue(TeamColorMaterialParameterName, TeamColor);
		DynamicMaterial->SetScalarParameterValue(TeamNumberMaterialParameterName, static_cast<float>(TeamNumber));
	}
}

void ARTSUnitBase::UpdateSelectionDecal()
{
    if (!SelectionDecalComponent)
    {
        return;
    }

    SelectionDecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    SelectionDecalComponent->SetRelativeLocation(FVector(0.0f, 0.0f, SelectionDecalZOffset));
    SelectionDecalComponent->DecalSize = SelectionDecalSize;

    if (SelectionDecalMaterial)
    {
        SelectionDecalComponent->SetDecalMaterial(SelectionDecalMaterial);
    }

    const bool bShowSelectionDecal = bIsSelected && SelectionDecalMaterial != nullptr;
    SelectionDecalComponent->SetVisibility(bShowSelectionDecal, true);
    SelectionDecalComponent->SetHiddenInGame(!bShowSelectionDecal, true);
}

void ARTSUnitBase::SetUnitData(URTSUnitData* NewUnitData)
{
	UnitData = NewUnitData;
	RefreshUnitVisual();
}

void ARTSUnitBase::RefreshUnitVisual()
{
	const bool bUseSkeletalMesh = UnitData && UnitData->UnitSkeletalMesh;

	if (SkeletalMeshComponent)
	{
		if (bUseSkeletalMesh)
		{
			SkeletalMeshComponent->SetSkeletalMesh(UnitData->UnitSkeletalMesh);
			SkeletalMeshComponent->SetAnimInstanceClass(UnitData->AnimationClass);
			SkeletalMeshComponent->SetHiddenInGame(false);
			SkeletalMeshComponent->SetVisibility(true);
			SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			SkeletalMeshComponent->SetCanEverAffectNavigation(true);

			for (int32 MaterialIndex = 0; MaterialIndex < UnitData->OverrideMaterials.Num(); ++MaterialIndex)
			{
				if (UnitData->OverrideMaterials[MaterialIndex])
				{
					SkeletalMeshComponent->SetMaterial(MaterialIndex, UnitData->OverrideMaterials[MaterialIndex]);
				}
			}
		}
		else
		{
			SkeletalMeshComponent->SetHiddenInGame(true);
			SkeletalMeshComponent->SetVisibility(false);


			SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SkeletalMeshComponent->SetCanEverAffectNavigation(false);
		}
	}

	if (MeshComponent)
	{
		MeshComponent->SetHiddenInGame(bUseSkeletalMesh);
		MeshComponent->SetVisibility(!bUseSkeletalMesh);
		MeshComponent->SetCollisionEnabled(bUseSkeletalMesh ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetCanEverAffectNavigation(!bUseSkeletalMesh);

		if (!bUseSkeletalMesh && UnitData)
		{
			for (int32 MaterialIndex = 0; MaterialIndex < UnitData->OverrideMaterials.Num(); ++MaterialIndex)
			{
				if (UnitData->OverrideMaterials[MaterialIndex])
				{
					MeshComponent->SetMaterial(MaterialIndex, UnitData->OverrideMaterials[MaterialIndex]);
				}
			}
		}
	}

	if (HealthComponent && UnitData)
	{
		HealthComponent->SetMaxHealth(UnitData->MaxHealth, HasAuthority());
	}
	if (UnitData)
	{
		MovementSpeed = FMath::Max(1.0f, UnitData->MovementSpeed);
		MovementAcceptanceRadius = FMath::Max(1.0f, UnitData->MovementAcceptanceRadius);
		SelectionDecalMaterial = UnitData->SelectionDecalMaterial;
		SelectionDecalSize = UnitData->SelectionDecalSize;
	}
	if (AttackComponent && UnitData)
	{
		AttackComponent->ConfigureAttackStats(
			UnitData->bCanAttack,
			UnitData->AttackDamage,
			UnitData->AttackRange,
			UnitData->AttackCooldown
		);
	}

	if (CombatEffectsComponent && UnitData)
	{
		CombatEffectsComponent->ConfigureFromUnitData(UnitData);
	}

	ApplyTeamVisual();
	UpdateSelectionDecal();
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

    if (UnitMovementComponent)
    {
        UnitMovementComponent->IssueMoveCommand(TargetLocation);
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

    if (UnitMovementComponent)
    {
        UnitMovementComponent->StopMovement();
        return;
    }

    bHasMoveTarget = false;
    SetActorTickEnabled(false);
}

void ARTSUnitBase::StopAllCommands()
{
    if (!HasAuthority())
    {
        return;
    }

    if (AttackComponent)
    {
        AttackComponent->StopAttackCommand();
    }

    StopMovement();
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

//명령 받을 ???�는지
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
    UpdateSelectionDecal();
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
