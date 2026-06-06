// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Core/RTSPlayerState.h"
#include "RTSRaceStartData.generated.h"

class ARTSUnitBase;
class ARTSBuilding;
class URTSBuildingData;
class URTSUnitData;
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Base")
	TObjectPtr<URTSBuildingData> MainBaseBuildingData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<ARTSUnitBase> WorkerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Workers")
	TObjectPtr<URTSUnitData> WorkerUnitData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 InitialWorkerCount = 12;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resources")
	int32 InitialMinerals = 50;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Resources")
	int32 InitialVespene = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Supply")
	int32 InitialSupplyCap = 15;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Supply")
	int32 InitialWorkerSupplyCost = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zerg")
	TSubclassOf<AActor> InitialCreepActorClass;
};
