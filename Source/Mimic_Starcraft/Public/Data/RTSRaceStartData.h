// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/RTSPlayerState.h"
#include "RTSRaceStartData.generated.h"

class ARTSUnitBase;
class ARTSBuilding;
class AActor;

UCLASS(BlueprintType)
class MIMIC_STARCRAFT_API URTSRaceStartData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ERTSRace Race = ERTSRace::Terran;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<ARTSBuilding> MainBaseClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<ARTSUnitBase> WorkerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 InitialWorkerCount = 12;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zerg")
	TSubclassOf<AActor> InitialCreepActorClass;
};
