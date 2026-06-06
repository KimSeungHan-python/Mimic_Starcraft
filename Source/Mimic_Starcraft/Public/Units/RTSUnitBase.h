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
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UMeshComponent;

UCLASS()
class MIMIC_STARCRAFT_API ARTSUnitBase : public AActor, public IRTSSelectableInterface
{
	GENERATED_BODY()
	
public:
	ARTSUnitBase();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	// Start and ownership state

	UPROPERTY(ReplicatedUsing = OnRep_TeamInfo, BlueprintReadOnly, Category = "RTS Team")
	int32 TeamNumber = -1;

	UPROPERTY(ReplicatedUsing = OnRep_TeamInfo, BlueprintReadOnly, Category = "RTS Team")
	FLinearColor TeamColor = FLinearColor::White;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Team")
	TObjectPtr<ARTSPlayerState> OwningPlayerState;

	UPROPERTY(ReplicatedUsing = OnRep_UnitData, BlueprintReadOnly, Category = "RTS Unit")
	TObjectPtr<URTSUnitData> UnitData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Team|Visual")
	bool bApplyTeamColorToMaterials = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Team|Visual")
	FName TeamColorMaterialParameterName = TEXT("TeamColor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Team|Visual")
	FName TeamNumberMaterialParameterName = TEXT("TeamNumber");

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	bool bIsSelected = false;

	UFUNCTION()
	void OnRep_TeamInfo();

	UFUNCTION()
	void OnRep_UnitData();

	void ApplyTeamVisual();

	UFUNCTION(BlueprintCallable, Category = "RTS Unit")
	void SetUnitData(URTSUnitData* NewUnitData);

	UFUNCTION(BlueprintCallable, Category = "RTS Unit|Visual")
	void RefreshUnitVisual();

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

protected:
	void ApplyTeamVisualToMesh(UMeshComponent* TargetMesh);
};
