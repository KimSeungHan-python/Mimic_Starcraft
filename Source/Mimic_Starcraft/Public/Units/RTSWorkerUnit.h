#pragma once

#include "CoreMinimal.h"
#include "Units/RTSUnitBase.h"
#include "RTSWorkerUnit.generated.h"

class ARTSResourceNode;
class URTSGatherComponent;
class URTSWorkerBuildComponent;

UCLASS()
class MIMIC_STARCRAFT_API ARTSWorkerUnit : public ARTSUnitBase
{
    GENERATED_BODY()

public:
    ARTSWorkerUnit();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<URTSGatherComponent> GatherComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<URTSWorkerBuildComponent> BuildComponent;

    UFUNCTION(BlueprintCallable, Category = "RTS Worker")
    bool GatherFromResource(ARTSResourceNode* ResourceNode);

    virtual void StopAllCommands() override;

    UFUNCTION(BlueprintCallable, Category = "RTS Worker")
    void StopWorkerCommand();
};
