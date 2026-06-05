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

	UFUNCTION()
	void OnRep_TeamInfo();

	void SetTeamInfo(int32 InTeamNumber, const FLinearColor& InTeamColor, ERTSRace InRace, int32 InCampIndex);

protected:
	//
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
};
