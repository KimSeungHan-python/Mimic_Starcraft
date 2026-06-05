// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "Types/RTSGridTypes.h"
#include "RTSStartCamp.generated.h"

class ARTSGridManager;
class AActor;
/**
 * 
 */
UCLASS()
class MIMIC_STARCRAFT_API ARTSStartCamp : public APlayerStart
{
	GENERATED_BODY()
	
public:
	ARTSStartCamp(const FObjectInitializer& ObjectInitializer);

	virtual void OnConstruction(const FTransform& Transform) override;

public:
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "RTS Start|Grid")
	TObjectPtr<ARTSGridManager> GridManager;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "RTS Start|Grid")
	bool bSnapToGridInEditor = true;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RTS Start|Grid")
	FRTSGridCoord AnchorCellCoord;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "RTS Start|Base")
	FRTSGridCoord MainBaseOffsetCellCoord = FRTSGridCoord(0, 0);

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "RTS Start|Base")
	TObjectPtr<AActor> MineralCenterActor;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "RTS Start|Base")
	float MainBaseYawOffset = 0.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "RTS Start|Workers")
	TArray<FRTSGridCoord> WorkerOffsetCells;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "RTS Start|Camera")
	FVector CameraOffset = FVector(-800.f, 0.f, 1200.f);


	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RTS Start")
	int32 CampIndex = -1;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RTS Start")
	TArray<FTransform> WorkerLocalTransforms;

	//for zerg <- 여기서 저그를 위한 땅을 세팅하는게 맞나 싶다.
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RTS Start|Zerg")
	FTransform CreepCenterLocalTransform;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RTS Start|Zerg")
	float InitialCreepRadius = 900.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS Start")
	bool bOccupied = false;



public:
	FTransform GetMainBaseWorldTransform() const;
	FTransform GetWorkerWorldTransform(int32 Index) const;
	FTransform GetCameraWorldTransform() const;
	FTransform GetCreepCenterWorldTransform() const;

private:
	FRotator GetRotationFacingMinerals(const FVector& FromLocation) const;
};
