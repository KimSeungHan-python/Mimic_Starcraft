#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/RTSGridTypes.h"
#include "RTSUnitMovementComponent.generated.h"

class ARTSGridManager;
class ARTSUnitBase;

UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class MIMIC_STARCRAFT_API URTSUnitMovementComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URTSUnitMovementComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Movement")
    bool bUseGridPathfinding = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Movement")
    TObjectPtr<ARTSGridManager> GridManager = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Movement", meta = (ClampMin = "0"))
    int32 MaxPathSearchCells = 2048;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Movement", meta = (ClampMin = "0"))
    int32 NearestWalkableSearchRadiusCells = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Movement", meta = (ClampMin = "0"))
    float RepathTargetChangeThreshold = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Movement|Separation")
    bool bUseSimpleSeparation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Movement|Separation", meta = (ClampMin = "0"))
    float SeparationRadius = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Movement|Separation", meta = (ClampMin = "0"))
    float SeparationStrength = 0.7f;

    UFUNCTION(BlueprintCallable, Category = "RTS Movement")
    void IssueMoveCommand(const FVector& TargetLocation);

    UFUNCTION(BlueprintCallable, Category = "RTS Movement")
    void StopMovement();

    UFUNCTION(BlueprintPure, Category = "RTS Movement")
    bool HasMoveCommand() const { return bHasMoveCommand; }

    UFUNCTION(BlueprintPure, Category = "RTS Movement")
    TArray<FVector> GetCurrentPathPoints() const { return PathPoints; }

protected:
    UPROPERTY(BlueprintReadOnly, Category = "RTS Movement")
    bool bHasMoveCommand = false;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Movement")
    FVector MoveTargetLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Movement")
    TArray<FVector> PathPoints;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Movement")
    int32 CurrentPathIndex = 0;

    ARTSUnitBase* GetOwningUnit() const;
    void ResolveGridManager();
    bool BuildPathTo(const FVector& TargetLocation);
    bool FindPathCells(FRTSGridCoord StartCoord, FRTSGridCoord GoalCoord, TArray<FRTSGridCoord>& OutPath) const;
    bool FindNearestWalkableCoord(FRTSGridCoord DesiredCoord, FRTSGridCoord& OutCoord) const;
    bool IsDiagonalMoveAllowed(FRTSGridCoord FromCoord, FRTSGridCoord ToCoord) const;
    FVector GetCurrentMoveTarget() const;
    FVector GetSeparationDirection() const;
    void MoveAlongPath(float DeltaTime);
    void SyncOwnerMoveState(bool bMoving);
};
