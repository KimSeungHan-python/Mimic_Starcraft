#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTSProductionQueueComponent.generated.h"

class ARTSBuilding;
class ARTSPlayerState;
class URTSUnitData;

USTRUCT(BlueprintType)
struct FRTSProductionQueueItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RTS Production")
    TObjectPtr<URTSUnitData> UnitData = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Production")
    float StartServerTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Production")
    float Duration = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRTSProductionQueueChanged);

UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class MIMIC_STARCRAFT_API URTSProductionQueueComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URTSProductionQueueComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Production")
    int32 MaxQueueSize = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Production|Spawn")
    FVector SpawnPointLocalOffset = FVector(300.0f, 0.0f, 0.0f);

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Production|Rally")
    bool bUseRallyPoint = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Production|Rally")
    FVector RallyPointWorldLocation = FVector::ZeroVector;

    UPROPERTY(ReplicatedUsing = OnRep_ProductionQueue, BlueprintReadOnly, Category = "RTS Production")
    TArray<FRTSProductionQueueItem> ProductionQueue;

    UPROPERTY(BlueprintAssignable, Category = "RTS Production")
    FRTSProductionQueueChanged OnProductionQueueChanged;

    UFUNCTION(BlueprintCallable, Category = "RTS Production")
    bool CanQueueUnit(URTSUnitData* UnitData, ARTSPlayerState* PlayerState) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Production")
    bool QueueUnit(URTSUnitData* UnitData, ARTSPlayerState* PlayerState);

    UFUNCTION(BlueprintCallable, Category = "RTS Production")
    bool CancelQueuedUnit(int32 QueueIndex, bool bRefundResources);

    UFUNCTION(BlueprintCallable, Category = "RTS Production")
    bool HasQueuedProduction() const;

    UFUNCTION(BlueprintCallable, Category = "RTS Production")
    float GetCurrentProductionProgress01() const;

    UFUNCTION(BlueprintCallable, Category = "RTS Production")
    FTransform GetSpawnTransform() const;

    UFUNCTION(BlueprintCallable, Category = "RTS Production|Rally")
    void SetRallyPoint(const FVector& WorldLocation);

    UFUNCTION(BlueprintCallable, Category = "RTS Production|Rally")
    void ClearRallyPoint();

    UFUNCTION(BlueprintCallable, Category = "RTS Production|Rally")
    bool HasRallyPoint() const;

protected:
    UFUNCTION()
    void OnRep_ProductionQueue();

private:
    ARTSBuilding* GetOwningBuilding() const;
    ARTSPlayerState* ResolvePlayerState(ARTSPlayerState* ExplicitPlayerState) const;
    float GetServerTimeSeconds() const;
    void ProcessProduction();
    void FinishCurrentProduction();
    void StartNextQueueItemIfNeeded();
    void BroadcastQueueChanged();
};
