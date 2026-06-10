#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/RTSGridTypes.h"
#include "UObject/ObjectKey.h"
#include "RTSActorSpatialIndex.generated.h"

class ARTSGridManager;

struct FRTSSpatialActorEntry
{
    TWeakObjectPtr<AActor> Actor;
    FRTSGridCoord Cell;
};

UCLASS()
class MIMIC_STARCRAFT_API ARTSActorSpatialIndex : public AActor
{
    GENERATED_BODY()

public:
    ARTSActorSpatialIndex();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Spatial")
    TObjectPtr<ARTSGridManager> GridManager = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Spatial", meta = (ClampMin = "1"))
    float SpatialCellSize = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Spatial", meta = (ClampMin = "0.01"))
    float UpdateInterval = 0.1f;

    UFUNCTION(BlueprintCallable, Category = "RTS Spatial")
    void RegisterActor(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "RTS Spatial")
    void UnregisterActor(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "RTS Spatial")
    void QueryActorsInRadius(const FVector& WorldLocation, float Radius, TArray<AActor*>& OutActors) const;

    UFUNCTION(BlueprintPure, Category = "RTS Spatial")
    int32 GetRegisteredActorCount() const { return ActorEntries.Num(); }

    static ARTSActorSpatialIndex* FindExisting(UWorld* World);
    static ARTSActorSpatialIndex* GetOrCreate(UWorld* World);

protected:
    FRTSGridCoord WorldToSpatialCell(const FVector& WorldLocation) const;
    float GetEffectiveCellSize() const;
    void ResolveGridManager();
    void AddToBucket(const TObjectKey<AActor>& ActorKey, FRTSGridCoord Cell);
    void RemoveFromBucket(const TObjectKey<AActor>& ActorKey, FRTSGridCoord Cell);
    void UpdateActorCell(const TObjectKey<AActor>& ActorKey, FRTSSpatialActorEntry& Entry);

private:
    TMap<TObjectKey<AActor>, FRTSSpatialActorEntry> ActorEntries;
    TMap<FRTSGridCoord, TArray<TObjectKey<AActor>>> Buckets;
};
