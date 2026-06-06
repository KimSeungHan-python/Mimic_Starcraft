#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Resources/RTSResourceNode.h"
#include "RTSGatherComponent.generated.h"

class ARTSBuilding;
class ARTSPlayerState;
class ARTSUnitBase;

UENUM(BlueprintType)
enum class ERTSGatherState : uint8
{
    Idle,
    MovingToResource,
    Gathering,
    ReturningToDropOff
};

UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class MIMIC_STARCRAFT_API URTSGatherComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URTSGatherComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Gather")
    int32 CarryCapacity = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Gather")
    float HarvestDuration = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Gather")
    float InteractionRadius = 120.0f;

    UPROPERTY(ReplicatedUsing = OnRep_GatherState, BlueprintReadOnly, Category = "RTS Gather")
    ERTSGatherState GatherState = ERTSGatherState::Idle;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Gather")
    TObjectPtr<ARTSResourceNode> TargetResourceNode;

    UFUNCTION(BlueprintCallable, Category = "RTS Gather")
    bool StartGathering(ARTSResourceNode* ResourceNode);

    UFUNCTION(BlueprintCallable, Category = "RTS Gather")
    void StopGathering();

    UFUNCTION(BlueprintCallable, Category = "RTS Gather")
    bool IsCarryingResources() const;

protected:
    UFUNCTION()
    void OnRep_GatherState();

private:
    ARTSUnitBase* GetOwningUnit() const;
    ARTSPlayerState* GetOwningPlayerState() const;
    ARTSBuilding* FindNearestDropOff(ERTSResourceType ResourceType) const;
    bool IsNearLocation(const FVector& Location) const;
    void SetGatherState(ERTSGatherState NewState);
    void MoveTowardResource();
    void MoveTowardDropOff();
    void ProcessMovingToResource();
    void ProcessGathering(float DeltaTime);
    void ProcessReturningToDropOff();
    void DepositCarriedResources();

    UPROPERTY()
    TObjectPtr<ARTSBuilding> CurrentDropOffBuilding;

    float HarvestTimer = 0.0f;
    int32 CarriedMinerals = 0;
    int32 CarriedVespene = 0;
};
