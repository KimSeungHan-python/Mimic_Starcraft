// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Buildings/RTSBuilding.h"
#include "RTSProtossBuilding.generated.h"

/**
 * 
 */
UCLASS()
class MIMIC_STARCRAFT_API ARTSProtossBuilding : public ARTSBuilding
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "RTS Building|Protoss")
    bool RequiresPower() const;

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Protoss")
    bool ProvidesPower() const;
};
