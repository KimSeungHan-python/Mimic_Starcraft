// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/RTSPlayerState.h"
#include "RTSGameModeBase.generated.h"


class ARTSStartCamp;
class URTSRaceStartData;
class ARTSPlayerController;

UCLASS()
class MIMIC_STARCRAFT_API ARTSGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	ARTSGameModeBase();

protected:
	virtual void BeginPlay() override;

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "RTS Start")
	TMap<ERTSRace, TObjectPtr<URTSRaceStartData>> RaceStartDataMap;

	UPROPERTY(EditDefaultsOnly, Category = "RTS Team")
	TArray<FLinearColor> TeamColors;

	UPROPERTY()
	TArray<TObjectPtr<ARTSStartCamp>> StartCamps;

	UPROPERTY()
	TMap<TObjectPtr<AController>, TObjectPtr<ARTSStartCamp>> AssignedCampMap;


private:
	void CollectStartCampsIfNeeded();

	ARTSStartCamp* AssignStartCamp(AController* Controller);

	void InitializePlayerState(APlayerController* NewPlayer, ARTSStartCamp* Camp);

	void SpawnInitialBaseAndWorkers(APlayerController* NewPlayer, ARTSStartCamp* Camp);

	void ApplyRaceStartEffect(APlayerController* NewPlayer, ARTSStartCamp* Camp, URTSRaceStartData* StartData);

	URTSRaceStartData* GetStartData(ERTSRace Race) const;

	ERTSRace GetRaceForPlayer(APlayerController* NewPlayer) const;
};
