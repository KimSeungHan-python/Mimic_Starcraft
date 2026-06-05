// Fill out your copyright notice in the Description page of Project Settings.


#include "Races/Protoss/RTSProtossBuilding.h"
#include "Data/RTSBuildingData.h"

bool ARTSProtossBuilding::RequiresPower() const
{
    return BuildingData && BuildingData->bRequiresPower;
}

bool ARTSProtossBuilding::ProvidesPower() const
{
    return BuildingData && BuildingData->bProvidesPower;
}