// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSZergBuilding.h"
#include "RTSBuildingData.h"

bool ARTSZergBuilding::RequiresCreep() const
{
    return BuildingData && BuildingData->bRequiresCreep;
}

bool ARTSZergBuilding::ProvidesCreep() const
{
    return BuildingData && BuildingData->bProvidesCreep;
}