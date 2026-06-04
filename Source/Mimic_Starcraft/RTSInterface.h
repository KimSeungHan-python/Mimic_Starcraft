// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RTSInterface.generated.h"

//class APlayerStartCamp;
class ARTSBuilding;
class ARTSUnitBase;
// This class does not need to be modified.

UINTERFACE(BlueprintType)
class MIMIC_STARCRAFT_API URTSInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MIMIC_STARCRAFT_API IRTSInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Selectable")
	void MarqueePressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Selectable")
	void MarqueeHeld();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Selectable")
	void MarqueeReleased();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Selectable")
	void SelectedThis_Decal();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Selectable")
	void DeselectedThis_Decal();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Selectable")
	TArray<AActor*> GrabSelectedUnits();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Selectable")
	void AddUnitToSelectedUnits(UUserWidget* Widget);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Selectable")
	void ClearSelectedUnits();

	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Setting")
	//void SetupPlayerStart(APlayerStartCamp* PlayerStartCamp, const int& TeamNumber,const FLinearColor& TeamColor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Setting")
	int GetTeam() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Setting")
	bool IsOnMyTeam(const int& TeamNumber) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Resources")
	int GetResources() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Resources")
	void IncrementResourceAmount(const int& ResourceAmounts);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Command")
	void ProduceUnit(ARTSBuilding* Building, ARTSUnitBase* Unit);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Command")
	void UnitMoveCommand(const FVector& Location);
};
