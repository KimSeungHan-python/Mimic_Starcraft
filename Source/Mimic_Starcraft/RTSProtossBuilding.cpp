// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSProtossBuilding.h"
#include "RTSBuildingData.h"

bool ARTSProtossBuilding::RequiresPower() const
{
    return BuildingData && BuildingData->bRequiresPower;
}

bool ARTSProtossBuilding::ProvidesPower() const
{
    return BuildingData && BuildingData->bProvidesPower;
}