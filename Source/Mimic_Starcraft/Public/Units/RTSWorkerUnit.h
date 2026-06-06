#pragma once

#include "CoreMinimal.h"
#include "Units/RTSUnitBase.h"
#include "RTSWorkerUnit.generated.h"

class ARTSResourceNode;
class URTSGatherComponent;

UCLASS()
class MIMIC_STARCRAFT_API ARTSWorkerUnit : public ARTSUnitBase
{
    GENERATED_BODY()

public:
    ARTSWorkerUnit();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<URTSGatherComponent> GatherComponent;

    UFUNCTION(BlueprintCallable, Category = "RTS Worker")
    bool GatherFromResource(ARTSResourceNode* ResourceNode);

    UFUNCTION(BlueprintCallable, Category = "RTS Worker")
    void StopWorkerCommand();
};
