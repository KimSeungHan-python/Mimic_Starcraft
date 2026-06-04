// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RTS_Resource_Interface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class URTS_Resource_Interface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MIMIC_STARCRAFT_API IRTS_Resource_Interface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS_Resource")
	void SetIsWorking(AActor* ResourceNode);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS_Resource")
	void SetNotWorking();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS_Resource")
	void Collect();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS_Resource")
	void DropOff();
};
