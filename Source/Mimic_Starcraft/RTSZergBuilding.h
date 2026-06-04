// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RTSBuilding.h"
#include "RTSZergBuilding.generated.h"

/**
 * 
 */
UCLASS()
class MIMIC_STARCRAFT_API ARTSZergBuilding : public ARTSBuilding
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "RTS Building|Zerg")
    bool RequiresCreep() const;

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Zerg")
    bool ProvidesCreep() const;
};
