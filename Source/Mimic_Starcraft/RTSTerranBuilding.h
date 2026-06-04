// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RTSBuilding.h"
#include "RTSTerranBuilding.generated.h"

/**
 * 
 */
UCLASS()
class MIMIC_STARCRAFT_API ARTSTerranBuilding : public ARTSBuilding
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "RTS Building|Terran")
    bool CanLiftOff() const;

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Terran")
    void LiftOff();

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Terran")
    bool CanLandAt(FRTSGridCoord TargetOriginCoord) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Terran")
    void LandAt(FRTSGridCoord TargetOriginCoord);
};
