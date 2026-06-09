// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Buildings/RTSBuilding.h"
#include "RTSTerranBuilding.generated.h"

/**
 * 
 */
UCLASS()
class MIMIC_STARCRAFT_API ARTSTerranBuilding : public ARTSBuilding
{
	GENERATED_BODY()
	
public:
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building|Terran|Flying")
    float FlyingMovementSpeed = 420.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building|Terran|Flying")
    float FlyingMovementAcceptanceRadius = 80.0f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Building|Terran|Flying")
    bool bHasFlyingMoveTarget = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Building|Terran|Flying")
    FVector FlyingMoveTargetLocation = FVector::ZeroVector;

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Terran")
    bool CanLiftOff() const;

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Terran")
    void LiftOff();

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Terran")
    bool IssueFlyingMoveCommand(const FVector& TargetLocation);

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Terran")
    void StopFlyingMovement();

    virtual void StopAllCommands() override;

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Terran")
    bool CanLandAt(FRTSGridCoord TargetOriginCoord) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Terran")
    void LandAt(FRTSGridCoord TargetOriginCoord);
};
