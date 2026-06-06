// Fill out your copyright notice in the Description page of Project Settings.

#include "Races/Terran/RTSTerranBuilding.h"

#include "Data/RTSBuildingData.h"
#include "Grid/RTSGridManager.h"

#include "Components/PrimitiveComponent.h"

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
