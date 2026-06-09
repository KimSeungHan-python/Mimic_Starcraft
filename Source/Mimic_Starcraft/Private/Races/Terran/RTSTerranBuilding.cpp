// Fill out your copyright notice in the Description page of Project Settings.

#include "Races/Terran/RTSTerranBuilding.h"

#include "Data/RTSBuildingData.h"
#include "Grid/RTSGridManager.h"

#include "Components/PrimitiveComponent.h"
#include "Net/UnrealNetwork.h"

void ARTSTerranBuilding::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!HasAuthority() || BuildingState != ERTSBuildingState::Flying || !bHasFlyingMoveTarget)
    {
        return;
    }

    const FVector CurrentLocation = GetActorLocation();
    FVector TargetLocation = FlyingMoveTargetLocation;
    TargetLocation.Z = CurrentLocation.Z;

    FVector Direction = TargetLocation - CurrentLocation;
    Direction.Z = 0.0f;

    const float Distance = Direction.Size();
    if (Distance <= FlyingMovementAcceptanceRadius)
    {
        StopFlyingMovement();
        return;
    }

    Direction /= Distance;
    const float Step = FMath::Min(Distance, FlyingMovementSpeed * DeltaTime);
    SetActorLocation(CurrentLocation + Direction * Step, true);
    SetActorRotation(Direction.Rotation());
}

void ARTSTerranBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARTSTerranBuilding, bHasFlyingMoveTarget);
    DOREPLIFETIME(ARTSTerranBuilding, FlyingMoveTargetLocation);
}

bool ARTSTerranBuilding::CanLiftOff() const
{
    return BuildingData
        && BuildingData->bCanLiftOff
        && BuildingState == ERTSBuildingState::Completed;
}

void ARTSTerranBuilding::LiftOff()
{
    if (!HasAuthority() || !CanLiftOff())
    {
        return;
    }

    if (!OwningGridManager)
    {
        OwningGridManager = ResolveGridManager();
    }

    if (!OwningGridManager)
    {
        return;
    }

    UnregisterFromGrid();

    SetActorLocation(
        GetActorLocation() + FVector(0.0f, 0.0f, BuildingData->LiftOffHeight)
    );

    BuildingState = ERTSBuildingState::Flying;
    StopFlyingMovement();

    TArray<UPrimitiveComponent*> PrimitiveComponents;
    GetComponents<UPrimitiveComponent>(PrimitiveComponents);

    for (UPrimitiveComponent* PrimComp : PrimitiveComponents)
    {
        if (!PrimComp)
        {
            continue;
        }

        PrimComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PrimComp->SetGenerateOverlapEvents(false);
        PrimComp->SetCanEverAffectNavigation(false);
    }

    OnRep_BuildingState();
}

bool ARTSTerranBuilding::IssueFlyingMoveCommand(const FVector& TargetLocation)
{
    if (!HasAuthority() || BuildingState != ERTSBuildingState::Flying)
    {
        return false;
    }

    FlyingMoveTargetLocation = TargetLocation;
    bHasFlyingMoveTarget = true;
    SetActorTickEnabled(true);
    return true;
}

void ARTSTerranBuilding::StopFlyingMovement()
{
    if (!HasAuthority())
    {
        return;
    }

    bHasFlyingMoveTarget = false;
    if (BuildingState != ERTSBuildingState::UnderConstruction)
    {
        SetActorTickEnabled(false);
    }
}

void ARTSTerranBuilding::StopAllCommands()
{
    if (!HasAuthority())
    {
        return;
    }

    StopFlyingMovement();
}

bool ARTSTerranBuilding::CanLandAt(FRTSGridCoord TargetOriginCoord) const
{
    if (!BuildingData || !OwningGridManager)
    {
        return false;
    }

    if (BuildingState != ERTSBuildingState::Flying)
    {
        return false;
    }

    return OwningGridManager->CanPlaceBuildingByData(
        TargetOriginCoord,
        BuildingData
    );
}

void ARTSTerranBuilding::LandAt(FRTSGridCoord TargetOriginCoord)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!OwningGridManager)
    {
        OwningGridManager = ResolveGridManager();
    }

    if (!CanLandAt(TargetOriginCoord))
    {
        return;
    }

    FVector LandLocation;
    if (!OwningGridManager->GetBuildingCenterLocationOnGround(
        TargetOriginCoord,
        GridWidth,
        GridHeight,
        LandLocation
    ))
    {
        return;
    }

    SetActorLocation(LandLocation);

    GridOriginCoord = TargetOriginCoord;
    BuildingState = ERTSBuildingState::Completed;
    StopFlyingMovement();
    RegisterToGrid();

    TArray<UPrimitiveComponent*> PrimitiveComponents;
    GetComponents<UPrimitiveComponent>(PrimitiveComponents);

    for (UPrimitiveComponent* PrimComp : PrimitiveComponents)
    {
        if (!PrimComp)
        {
            continue;
        }

        PrimComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        PrimComp->SetGenerateOverlapEvents(true);
        PrimComp->SetCanEverAffectNavigation(true);
    }

    OnRep_BuildingState();
}
