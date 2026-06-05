// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/RTSGridTypes.h"
#include "RTSVespeneGeyser.generated.h"

class ARTSGridManager;

UCLASS()
class MIMIC_STARCRAFT_API ARTSVespeneGeyser : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARTSVespeneGeyser();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Resource")
	ARTSGridManager* GridManager;

private:
	void RegisterToGrid();

};
