#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSResourceNode.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ERTSResourceType : uint8
{
    Minerals,
    Vespene
};

UCLASS()
class MIMIC_STARCRAFT_API ARTSResourceNode : public AActor
{
    GENERATED_BODY()

public:
    ARTSResourceNode();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Resource")
    ERTSResourceType ResourceType = ERTSResourceType::Minerals;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Resource")
    int32 MaxAmount = 1500;

    UPROPERTY(ReplicatedUsing = OnRep_RemainingAmount, BlueprintReadOnly, Category = "RTS Resource")
    int32 RemainingAmount = 1500;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Resource")
    FVector GatherPointLocalOffset = FVector::ZeroVector;

    UFUNCTION(BlueprintCallable, Category = "RTS Resource")
    bool HasResources() const;

    UFUNCTION(BlueprintCallable, Category = "RTS Resource")
    int32 Harvest(int32 RequestedAmount);

    UFUNCTION(BlueprintCallable, Category = "RTS Resource")
    FVector GetGatherLocation() const;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnRep_RemainingAmount();
};
