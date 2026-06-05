// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSUnitBase.generated.h"

class ARTSPlayerState;

UCLASS()
class MIMIC_STARCRAFT_API ARTSUnitBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARTSUnitBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	// Start 및 소유 관련 변수
public:

	UPROPERTY(ReplicatedUsing = OnRep_TeamInfo, BlueprintReadOnly, Category = "RTS Team")
	int32 TeamNumber = -1;

	UPROPERTY(ReplicatedUsing = OnRep_TeamInfo, BlueprintReadOnly, Category = "RTS Team")
	FLinearColor TeamColor = FLinearColor::White;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Team")
	TObjectPtr<ARTSPlayerState> OwningPlayerState;

	UFUNCTION()
	void OnRep_TeamInfo();

	void ApplyTeamVisual();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void SetTeamInfo(int32 NewTeamNumber, const FLinearColor& NewTeamColor);
};
