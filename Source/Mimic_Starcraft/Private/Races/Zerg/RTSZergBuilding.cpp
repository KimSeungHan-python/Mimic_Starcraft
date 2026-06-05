// Fill out your copyright notice in the Description page of Project Settings.


#include "Races/Zerg/RTSZergBuilding.h"
#include "Data/RTSBuildingData.h"

bool ARTSZergBuilding::RequiresCreep() const
{
    return BuildingData && BuildingData->bRequiresCreep;
}

bool ARTSZergBuilding::ProvidesCreep() const
{
    return BuildingData && BuildingData->bProvidesCreep;
}