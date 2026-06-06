// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "RTSPlayerState.generated.h"


UENUM(BlueprintType)
enum class ERTSRace : uint8
{
	Terran,
	Protoss,
	Zerg
};
UCLASS()
class MIMIC_STARCRAFT_API ARTSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ARTSPlayerState();

	UPROPERTY(ReplicatedUsing = OnRep_TeamInfo, BlueprintReadOnly, Category = "RTS Team")
	int32 TeamNumber = -1;

	UPROPERTY(ReplicatedUsing = OnRep_TeamInfo, BlueprintReadOnly, Category = "RTS Team")
	FLinearColor TeamColor = FLinearColor::White;

	UPROPERTY(ReplicatedUsing = OnRep_TeamInfo, BlueprintReadOnly, Category = "RTS Team")
	ERTSRace Race = ERTSRace::Terran;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Start")
	int32 AssignedCampIndex = -1;

	UPROPERTY(ReplicatedUsing = OnRep_Resources, BlueprintReadOnly, Category = "RTS Resources")
	int32 Minerals = 50;

	UPROPERTY(ReplicatedUsing = OnRep_Resources, BlueprintReadOnly, Category = "RTS Resources")
	int32 Vespene = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Supply, BlueprintReadOnly, Category = "RTS Supply")
	int32 SupplyUsed = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Supply, BlueprintReadOnly, Category = "RTS Supply")
	int32 SupplyCap = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RTS Supply")
	int32 MaxSupplyCap = 200;

	UFUNCTION()
	void OnRep_Resources();

	UFUNCTION()
	void OnRep_Supply();

	UFUNCTION(BlueprintCallable, Category = "RTS Resources")
	bool CanAfford(int32 MineralCost, int32 VespeneCost) const;

	UFUNCTION(BlueprintCallable, Category = "RTS Resources")
	bool TrySpendResources(int32 MineralCost, int32 VespeneCost);

	UFUNCTION(BlueprintCallable, Category = "RTS Resources")
	void AddResources(int32 MineralAmount, int32 VespeneAmount);

	void SetResources(int32 InMinerals, int32 InVespene);

	UFUNCTION(BlueprintCallable, Category = "RTS Supply")
	bool CanReserveSupply(int32 SupplyCost) const;

	UFUNCTION(BlueprintCallable, Category = "RTS Supply")
	bool TryReserveSupply(int32 SupplyCost);

	UFUNCTION(BlueprintCallable, Category = "RTS Supply")
	void ReleaseSupply(int32 SupplyAmount);

	UFUNCTION(BlueprintCallable, Category = "RTS Supply")
	void AddSupplyCap(int32 SupplyAmount);

	UFUNCTION(BlueprintCallable, Category = "RTS Supply")
	void RemoveSupplyCap(int32 SupplyAmount);

	void SetSupplyCap(int32 InSupplyCap);

	UFUNCTION()
	void OnRep_TeamInfo();

	void SetTeamInfo(int32 InTeamNumber, const FLinearColor& InTeamColor, ERTSRace InRace, int32 InCampIndex);

protected:
	//
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
};
