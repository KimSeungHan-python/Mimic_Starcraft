#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/RTSGridTypes.h"
#include "RTSWorkerBuildComponent.generated.h"

class ARTSGridManager;
class ARTSPlayerController;
class ARTSWorkerUnit;
class URTSBuildingData;

UENUM(BlueprintType)
enum class ERTSWorkerBuildState : uint8
{
    Idle,
    MovingToBuildSite
};

UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class MIMIC_STARCRAFT_API URTSWorkerBuildComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URTSWorkerBuildComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Build")
    float BuildStartAcceptanceRadius = 220.0f;

    UPROPERTY(ReplicatedUsing = OnRep_BuildState, BlueprintReadOnly, Category = "RTS Build")
    ERTSWorkerBuildState BuildState = ERTSWorkerBuildState::Idle;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Build")
    TObjectPtr<URTSBuildingData> TargetBuildingData = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Build")
    FRTSGridCoord TargetOriginCoord;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Build")
    FVector TargetBuildLocation = FVector::ZeroVector;

    UFUNCTION(BlueprintCallable, Category = "RTS Build")
    bool StartBuildOrder(
        ARTSPlayerController* BuildController,
        URTSBuildingData* BuildingData,
        FRTSGridCoord OriginCoord,
        ARTSGridManager* GridManager
    );

    UFUNCTION(BlueprintCallable, Category = "RTS Build")
    void StopBuildOrder();

    UFUNCTION(BlueprintCallable, Category = "RTS Build")
    bool HasActiveBuildOrder() const;

protected:
    UFUNCTION()
    void OnRep_BuildState();

private:
    UPROPERTY()
    TObjectPtr<ARTSPlayerController> OwningBuildController = nullptr;

    ARTSWorkerUnit* GetOwningWorker() const;
    void SetBuildState(ERTSWorkerBuildState NewState);
    bool IsAtBuildSite() const;
    void TryCompleteBuildOrder();
};
