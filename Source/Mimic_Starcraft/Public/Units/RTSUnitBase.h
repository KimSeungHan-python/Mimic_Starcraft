// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/RTSSelectableInterface.h"
#include "RTSUnitBase.generated.h"

class ARTSPlayerState;
class URTSUnitData;
class ARTSPlayerController;
class AController;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class MIMIC_STARCRAFT_API ARTSUnitBase : public AActor, public IRTSSelectableInterface
{
	GENERATED_BODY()
	
public:
	ARTSUnitBase();
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// Start and ownership state

	UPROPERTY(ReplicatedUsing = OnRep_TeamInfo, BlueprintReadOnly, Category = "RTS Team")
	int32 TeamNumber = -1;

	UPROPERTY(ReplicatedUsing = OnRep_TeamInfo, BlueprintReadOnly, Category = "RTS Team")
	FLinearColor TeamColor = FLinearColor::White;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Team")
	TObjectPtr<ARTSPlayerState> OwningPlayerState;

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	bool bIsSelected = false;

	UFUNCTION()
	void OnRep_TeamInfo();

	void ApplyTeamVisual();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void SetTeamInfo(int32 NewTeamNumber, const FLinearColor& NewTeamColor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Unit|Movement")
	float MovementSpeed = 550.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Unit|Movement")
	float MovementAcceptanceRadius = 45.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Unit|Movement")
	bool bHasMoveTarget = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Unit|Movement")
	FVector MoveTargetLocation = FVector::ZeroVector;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Unit|Supply")
	int32 SupplyCost = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Unit|Supply")
	bool bCountsTowardSupply = false;

	UFUNCTION(BlueprintCallable, Category = "RTS Unit|Movement")
	void IssueMoveCommand(const FVector& TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "RTS Unit|Movement")
	void StopMovement();

	UFUNCTION(BlueprintCallable, Category = "RTS Unit|Movement")
	bool HasReachedLocation(const FVector& TargetLocation, float AcceptanceRadius) const;

	UFUNCTION(BlueprintCallable, Category = "RTS Unit|Supply")
	bool RegisterSupplyCost(int32 InSupplyCost, bool bAlreadyReserved);

	bool CanReceiveCommandsFrom(AController* Controller) const;

	virtual bool CanBeSelectedBy_Implementation(ARTSPlayerController* SelectingController) const override;
	virtual void SetSelectionState_Implementation(bool bSelected) override;
	virtual bool IsSelected_Implementation() const override;
	virtual int32 GetSelectableTeamNumber_Implementation() const override;
	virtual bool IsOwnedByPlayerState_Implementation(ARTSPlayerState* PlayerState) const override;
};
