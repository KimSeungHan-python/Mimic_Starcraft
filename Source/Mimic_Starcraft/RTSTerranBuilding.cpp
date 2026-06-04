// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSTerranBuilding.h"
#include "RTSBuildingData.h"
#include "RTSGridManager.h"

bool ARTSTerranBuilding::CanLiftOff() const
{
    return BuildingData
        && BuildingData->bCanLiftOff
        && BuildingState == ERTSBuildingState::Completed
        && OwningGridManager;
}

void ARTSTerranBuilding::LiftOff()
{
    if (!CanLiftOff())
    {
        return;
    }

    OwningGridManager->ReleaseBuildingCells(
        GridOriginCoord,
        GridWidth,
        GridHeight
    );

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
        PrimComp->SetCanEverAffectNavigation(false);
    }
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
    if (!CanLandAt(TargetOriginCoord))
    {
        return;
    }

    FVector LandLocation;

    OwningGridManager->GetBuildingCenterLocationOnGround(
        TargetOriginCoord,
        GridWidth,
        GridHeight,
        LandLocation
    );

    SetActorLocation(LandLocation);

    GridOriginCoord = TargetOriginCoord;

    OwningGridManager->OccupyBuildingCells(
        GridOriginCoord,
        GridWidth,
        GridHeight,
        GetUniqueID()
    );

    BuildingState = ERTSBuildingState::Completed;

    TArray<UPrimitiveComponent*> PrimitiveComponents;
    GetComponents<UPrimitiveComponent>(PrimitiveComponents);

    for (UPrimitiveComponent* PrimComp : PrimitiveComponents)
    {
        if (!PrimComp)
        {
            continue;
        }

        PrimComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        PrimComp->SetCanEverAffectNavigation(true);
    }
}
