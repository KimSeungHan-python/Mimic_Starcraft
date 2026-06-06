#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Types/RTSGridTypes.h"
#include "RTSPlayerController.generated.h"

class ARTSGridManager;
class URTSBuildingData;
class URTSUnitData;
class ARTSBuilding;
class ARTSBuildGridPreview;
class ARTSResourceNode;
class ARTSUnitBase;
class ARTSWorkerUnit;
class AActor;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class MIMIC_STARCRAFT_API ARTSPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ARTSPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building")
    ARTSGridManager* GridManager;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building")
    URTSBuildingData* SelectedBuildingData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building")
    TSubclassOf<ARTSBuilding> DefaultBuildingClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building")
    TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

    // 서버 RPC에서 BuildingId로 DataAsset을 다시 찾기 위한 목록
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Building")
    TArray<URTSBuildingData*> BuildingDataList;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Production")
    TArray<URTSUnitData*> UnitDataList;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    bool bIsInBuildMode = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building|Preview")
    UMaterialInterface* ValidPreviewMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building|Preview")
    UMaterialInterface* InvalidPreviewMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building|Preview")
    TSubclassOf<ARTSBuildGridPreview> BuildGridPreviewClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection")
    TEnumAsByte<ECollisionChannel> SelectionTraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection")
    float DragSelectThresholdPixels = 8.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
    TArray<TObjectPtr<AActor>> SelectedActors;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
    TObjectPtr<UInputMappingContext> PlayerMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
    TObjectPtr<UInputAction> ConfirmBuildAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
    TObjectPtr<UInputAction> CancelBuildAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
    TObjectPtr<UInputAction> SelectAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
    TObjectPtr<UInputAction> CommandAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
    int32 PlayerMappingPriority = 1;

    void AddPlayerMappingContext();

    void HandleConfirmBuild(const FInputActionValue& Value);
    void HandleCancelBuild(const FInputActionValue& Value);
    void HandleSelectStarted(const FInputActionValue& Value);
    void HandleSelectCompleted(const FInputActionValue& Value);
    void HandleCommand(const FInputActionValue& Value);

private:
    UPROPERTY()
    ARTSBuilding* PreviewBuildingActor = nullptr;

    UPROPERTY()
    FTransform LastPreviewTransform;

    UPROPERTY()
    bool bHasValidPreviewTransform = false;

    FRTSGridCoord CurrentPreviewCoord;
    bool bCurrentPlacementValid = false;

    UPROPERTY()
    ARTSBuildGridPreview* BuildGridPreviewActor = nullptr;

public:
    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void StartBuildMode(URTSBuildingData* BuildingData);

    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void CancelBuildMode();

    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void ConfirmBuild();

    UFUNCTION(Client, Reliable)
    void Client_SetStartCamera(const FTransform& CameraTransform);

    ARTSGridManager* ResolveGridManager();

    UFUNCTION(BlueprintCallable, Category = "RTS Selection")
    void ClearSelection();

    UFUNCTION(BlueprintCallable, Category = "RTS Selection")
    void SelectActor(AActor* Actor, bool bAppendSelection);

    UFUNCTION(BlueprintCallable, Category = "RTS Production")
    bool QueueProductionForSelectedBuilding(URTSUnitData* UnitData);

    UFUNCTION(BlueprintCallable, Category = "RTS Command")
    void IssueSmartCommand();

    const TArray<TObjectPtr<AActor>>& GetSelectedActors() const { return SelectedActors; }
    bool IsSelectionDragging() const { return bIsDraggingSelection; }
    FVector2D GetSelectionDragStart() const { return SelectionDragStart; }
    FVector2D GetSelectionDragEnd() const { return SelectionDragEnd; }

protected:
    UFUNCTION(Server, Reliable)
    void ServerConfirmBuild(FName BuildingId, FRTSGridCoord OriginCoord);

    UFUNCTION(Server, Reliable)
    void ServerQueueProduction(ARTSBuilding* Building, FName UnitId);

    UFUNCTION(Server, Reliable)
    void ServerIssueMoveCommand(const TArray<ARTSUnitBase*>& Units, FVector TargetLocation);

    UFUNCTION(Server, Reliable)
    void ServerIssueGatherCommand(const TArray<ARTSWorkerUnit*>& Workers, ARTSResourceNode* ResourceNode);

private:
    bool GetMouseWorldLocation(FVector& OutLocation) const;
    void BeginSelection();
    void UpdateSelectionDrag();
    void EndSelection();
    void SelectSingleActorUnderCursor(bool bAppendSelection);
    void SelectActorsInScreenRect(const FVector2D& StartScreen, const FVector2D& EndScreen, bool bAppendSelection);
    bool IsActorSelectable(AActor* Actor) const;
    bool IsOwnedByLocalPlayer(AActor* Actor) const;
    bool IsActorVisibleOnScreen(AActor* Actor) const;
    void SelectVisibleActorsOfSameClass(AActor* SourceActor, bool bAppendSelection);
    TArray<ARTSUnitBase*> GetOwnedSelectedUnits() const;
    TArray<ARTSWorkerUnit*> GetOwnedSelectedWorkers() const;
    void UpdateBuildingPreview();
    void CreatePreviewActor();
    void DestroyPreviewActor();
    void SetPreviewValidVisual(bool bValid);
    void CreateBuildGridPreviewActor();
    void DestroyBuildGridPreviewActor();

    void BuildOnServer(FName BuildingId, FRTSGridCoord OriginCoord);
    bool QueueProductionOnServer(ARTSBuilding* Building, FName UnitId);
    void IssueMoveCommandOnServer(const TArray<ARTSUnitBase*>& Units, const FVector& TargetLocation);
    void IssueGatherCommandOnServer(const TArray<ARTSWorkerUnit*>& Workers, ARTSResourceNode* ResourceNode);
    ARTSBuilding* FindFirstOwnedSelectedBuilding() const;
    URTSBuildingData* FindBuildingDataById(FName BuildingId) const;
    URTSUnitData* FindUnitDataById(FName UnitId) const;

    bool bIsDraggingSelection = false;
    FVector2D SelectionDragStart = FVector2D::ZeroVector;
    FVector2D SelectionDragEnd = FVector2D::ZeroVector;
};