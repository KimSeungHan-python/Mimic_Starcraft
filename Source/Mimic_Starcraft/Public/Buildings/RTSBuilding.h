#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/RTSGridTypes.h"
#include "Interfaces/RTSSelectableInterface.h"
#include "RTSBuilding.generated.h"

class URTSBuildingData;
class URTSUnitData;
class URTSProductionQueueComponent;
class ARTSGridManager;
class ARTSPlayerState;
class UStaticMeshComponent;
class ARTSPlayerController;
class AController;
class USceneComponent;

UENUM(BlueprintType)
enum class ERTSBuildingState : uint8
{
    Preview,
    UnderConstruction,
    Completed,
    Flying
};

UCLASS()
class MIMIC_STARCRAFT_API ARTSBuilding : public AActor, public IRTSSelectableInterface
{
    GENERATED_BODY()

public:
    ARTSBuilding();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<URTSProductionQueueComponent> ProductionQueueComponent;

    UPROPERTY(ReplicatedUsing = OnRep_BuildingSetup, BlueprintReadOnly, Category = "RTS Building")
    URTSBuildingData* BuildingData;

    UPROPERTY(ReplicatedUsing = OnRep_BuildingSetup, BlueprintReadOnly, Category = "RTS Building")
    FRTSGridCoord GridOriginCoord;

    UPROPERTY(ReplicatedUsing = OnRep_BuildingSetup, BlueprintReadOnly, Category = "RTS Building")
    int32 GridWidth = 1;

    UPROPERTY(ReplicatedUsing = OnRep_BuildingSetup, BlueprintReadOnly, Category = "RTS Building")
    int32 GridHeight = 1;

    UPROPERTY(ReplicatedUsing = OnRep_BuildingSetup, BlueprintReadOnly, Category = "RTS Building")
    float CachedCellSize = 100.0f;

    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void InitializeBuilding(
        URTSBuildingData* InBuildingData,
        FRTSGridCoord InGridOriginCoord,
        int32 InGridWidth,
        int32 InGridHeight,
        float InCellSize,
        ARTSGridManager* InGridManager
    );

    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void FitMeshToGridFootprint(float CellSize);

 
    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void SetPreviewBuildingMode(bool bPreview);

    //건물 추가 Update들 
    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    TObjectPtr<ARTSGridManager> OwningGridManager = nullptr;

    
    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void SetOwningGridManager(ARTSGridManager* InGridManager);

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Grid")
    void RegisterToGrid();

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Grid")
    void UnregisterFromGrid();


    UPROPERTY(ReplicatedUsing = OnRep_BuildingState, BlueprintReadOnly, Category = "RTS Building|Construction")
    ERTSBuildingState BuildingState = ERTSBuildingState::Completed;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Building|Construction")
    float BuildTime = 0.0f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Building|Construction")
    float BuildStartServerTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building|Construction")
    float CurrentBuildTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building|Construction")
    bool bIsCompleted = true;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building|Preview")
    bool bIsPreviewBuilding = false;

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Construction")
    void BeginConstruction(float InBuildTime);

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Construction")
    void CompleteConstruction();

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Construction")
    void CancelConstruction(bool bRefundResources);

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Construction")
    float GetBuildProgress01() const;

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Production")
    bool QueueUnitProduction(URTSUnitData* UnitData);

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Production")
    void SetProductionRallyPoint(const FVector& WorldLocation);

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Production")
    void ClearProductionRallyPoint();

    UFUNCTION(BlueprintNativeEvent, Category = "RTS Building|Construction")
    void OnConstructionCompleted();

    UFUNCTION()
    void OnRep_BuildingSetup();

    UFUNCTION()
    void OnRep_BuildingState();

private:
    bool bRegisteredOnLocalGrid = false;
    bool bCompletedGridEffectsApplied = false;

protected:
    void RefreshBuildingVisual();
    void RegisterToLocalGridIfNeeded();
    void ApplyCompletedGridEffectsIfNeeded();
    void RemoveCompletedGridEffectsIfNeeded();
    FRTSGridCoord GetFootprintCenterCoord() const;
    ARTSGridManager* ResolveGridManager();
    float GetSyncedServerTimeSeconds() const;

    // Start 및 소유 관련 변수
public:

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

    bool CanReceiveCommandsFrom(AController* Controller) const;

    virtual bool CanBeSelectedBy_Implementation(ARTSPlayerController* SelectingController) const override;
    virtual void SetSelectionState_Implementation(bool bSelected) override;
    virtual bool IsSelected_Implementation() const override;
    virtual int32 GetSelectableTeamNumber_Implementation() const override;
    virtual bool IsOwnedByPlayerState_Implementation(ARTSPlayerState* PlayerState) const override;
};
