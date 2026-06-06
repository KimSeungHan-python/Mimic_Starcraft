// Fill out your copyright notice in the Description page of Project Settings.


#include "Resources/RTSVespeneGeyser.h"
#include "Grid/RTSGridManager.h"
#include "EngineUtils.h"

// Sets default values
ARTSVespeneGeyser::ARTSVespeneGeyser()
{
    PrimaryActorTick.bCanEverTick = false;
    ResourceType = ERTSResourceType::Vespene;
    MaxAmount = 2250;
    RemainingAmount = MaxAmount;
    bBlocksBuildingPlacement = false;
}

void ARTSVespeneGeyser::BeginPlay()
{
    Super::BeginPlay();

    RegisterToGrid();
}

void ARTSVespeneGeyser::RegisterToGrid()
{
    if (!GetWorld())
    {
        return;
    }

    if (!GridManager)
    {
        for (TActorIterator<ARTSGridManager> It(GetWorld()); It; ++It)
        {
            GridManager = *It;
            break;
        }
    }

    if (!GridManager)
    {
        return;
    }

    const FRTSGridCoord Coord = GridManager->WorldToGrid(GetActorLocation());

    GridManager->MarkVespeneGeyser(Coord, true);
}

