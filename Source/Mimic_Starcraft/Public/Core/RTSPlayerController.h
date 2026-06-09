#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Types/RTSCommandTypes.h"
#include "Types/RTSGridTypes.h"
#include "RTSPlayerController.generated.h"

class ARTSGridManager;
class URTSBuildingData;
class URTSUnitData;
class ARTSBuilding;
class ARTSBuildGridPreview;
class ARTSResourceNode;
class ARTSTerranBuilding;
class ARTSUnitBase;
class ARTSVisionManager;
class ARTSWorkerUnit;
class AActor;
class UInputMappingContext;
class UInputAction;
class UTexture2D;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRTSSelectionChanged);

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command")
    int32 MaxCommandCardSlots = 24;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Utility")
    FKey StopCommandHotkey = EKeys::S;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Utility")
    TObjectPtr<UTexture2D> StopCommandIcon = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    FName ActiveCommandPage = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Hotkeys")
    bool bEnableDefaultCommandHotkeys = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Hotkeys")
    TArray<FKey> DefaultCommandHotkeys;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Control Group")
    float ControlGroupDoubleTapSeconds = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Control Group")
    bool bEnableDefaultControlGroupHotkeys = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building")
    float WorkerBuildStartAcceptanceRadius = 220.0f;

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Vision")
    TObjectPtr<ARTSVisionManager> VisionManager = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Vision")
    TSubclassOf<ARTSVisionManager> VisionManagerClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Vision")
    bool bUseVisionForEnemyVisibility = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Feedback")
    bool bShowCommandFeedback = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Feedback")
    float CommandFeedbackDuration = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Feedback")
    float CommandFeedbackRadius = 85.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Feedback")
    TObjectPtr<UMaterialInterface> GroundCommandDecalMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Feedback")
    TObjectPtr<UMaterialInterface> TargetCommandDecalMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Feedback")
    TObjectPtr<UMaterialInterface> AttackCommandDecalMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Feedback")
    FVector CommandFeedbackDecalSize = FVector(120.0f, 120.0f, 120.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Attack")
    bool bUseAttackCommandModifier = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command|Attack")
    FKey AttackCommandModifierKey = EKeys::A;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
    TArray<TObjectPtr<AActor>> SelectedActors;

    UPROPERTY(BlueprintAssignable, Category = "RTS Selection")
    FRTSSelectionChanged OnSelectionChanged;

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
    void HandleKeyboardShortcuts();

private:
    UPROPERTY()
    TMap<int32, FRTSControlGroup> ControlGroups;

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

    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    bool CompleteWorkerBuildOrder(ARTSWorkerUnit* Worker, FName BuildingId, FRTSGridCoord OriginCoord);

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
    void GetAvailableCommandButtons(TArray<FRTSCommandButton>& OutCommands) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Command")
    bool ExecuteCommandButton(const FRTSCommandButton& Command);

    UFUNCTION(BlueprintCallable, Category = "RTS Command")
    bool ExecuteCommandSlot(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "RTS Command")
    bool ExecuteCommandHotkey(FKey Hotkey);

    UFUNCTION(BlueprintCallable, Category = "RTS Command")
    void SetActiveCommandPage(FName CommandPage);

    UFUNCTION(BlueprintCallable, Category = "RTS Command")
    void ClearCommandPage();

    UFUNCTION(BlueprintCallable, Category = "RTS Command")
    void IssueSmartCommand();

    UFUNCTION(BlueprintCallable, Category = "RTS Command")
    bool StopSelectedCommands();

    UFUNCTION(BlueprintCallable, Category = "RTS Vision")
    ARTSVisionManager* ResolveVisionManager();

    UFUNCTION(BlueprintPure, Category = "RTS Vision")
    bool IsActorVisibleToLocalPlayer(AActor* Actor) const;

    UFUNCTION(BlueprintPure, Category = "RTS Vision")
    bool IsWorldLocationVisibleToLocalPlayer(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintPure, Category = "RTS Vision")
    bool IsWorldLocationExploredByLocalPlayer(const FVector& WorldLocation) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Control Group")
    void HandleControlGroupInput(int32 GroupIndex);

    UFUNCTION(BlueprintCallable, Category = "RTS Control Group")
    void AssignControlGroup(int32 GroupIndex);

    UFUNCTION(BlueprintCallable, Category = "RTS Control Group")
    void AddSelectionToControlGroup(int32 GroupIndex);

    UFUNCTION(BlueprintCallable, Category = "RTS Control Group")
    bool RecallControlGroup(int32 GroupIndex, bool bAppendSelection = false);

    UFUNCTION(BlueprintCallable, Category = "RTS Control Group")
    void ClearControlGroup(int32 GroupIndex);

    const TArray<TObjectPtr<AActor>>& GetSelectedActors() const { return SelectedActors; }
    bool IsSelectionDragging() const { return bIsDraggingSelection; }

    //현재 마우스 위치를 리턴함 
    FVector2D GetSelectionDragStart() const { return SelectionDragStart; }
    FVector2D GetSelectionDragEnd() const { return SelectionDragEnd; }

protected:
    UFUNCTION(Server, Reliable)
    void ServerConfirmBuild(FName BuildingId, FRTSGridCoord OriginCoord);

    UFUNCTION(Server, Reliable)
    void ServerStartWorkerBuild(ARTSWorkerUnit* Worker, FName BuildingId, FRTSGridCoord OriginCoord);

    UFUNCTION(Server, Reliable)
    void ServerQueueProduction(ARTSBuilding* Building, FName UnitId);

    UFUNCTION(Server, Reliable)
    void ServerIssueMoveCommand(const TArray<ARTSUnitBase*>& Units, FVector TargetLocation);

    UFUNCTION(Server, Reliable)
    void ServerIssueAttackTargetCommand(const TArray<ARTSUnitBase*>& Units, AActor* TargetActor);

    UFUNCTION(Server, Reliable)
    void ServerIssueAttackMoveCommand(const TArray<ARTSUnitBase*>& Units, FVector TargetLocation);

    UFUNCTION(Server, Reliable)
    void ServerStopCommands(const TArray<ARTSUnitBase*>& Units, const TArray<ARTSBuilding*>& Buildings);

    UFUNCTION(Server, Reliable)
    void ServerIssueFlyingBuildingMoveCommand(const TArray<ARTSTerranBuilding*>& Buildings, FVector TargetLocation);

    UFUNCTION(Server, Reliable)
    void ServerIssueGatherCommand(const TArray<ARTSWorkerUnit*>& Workers, ARTSResourceNode* ResourceNode);

    UFUNCTION(Server, Reliable)
    void ServerSetRallyPoint(const TArray<ARTSBuilding*>& Buildings, FVector TargetLocation);

private:
    bool GetMouseWorldLocation(FVector& OutLocation) const;
    void BeginSelection();
    void UpdateSelectionDrag();
    void EndSelection();
    void BroadcastSelectionChanged();
    void SelectSingleActorUnderCursor(bool bAppendSelection);
    void SelectActorsInScreenRect(const FVector2D& StartScreen, const FVector2D& EndScreen, bool bAppendSelection);
    bool IsActorSelectable(AActor* Actor) const;
    bool IsOwnedByLocalPlayer(AActor* Actor) const;
    bool IsActorVisibleOnScreen(AActor* Actor) const;
    void SelectVisibleActorsOfSameClass(AActor* SourceActor, bool bAppendSelection);
    TArray<ARTSUnitBase*> GetOwnedSelectedUnits() const;
    TArray<ARTSWorkerUnit*> GetOwnedSelectedWorkers() const;
    TArray<ARTSBuilding*> GetOwnedSelectedBuildings() const;
    TArray<ARTSTerranBuilding*> GetOwnedSelectedFlyingTerranBuildings() const;
    ARTSWorkerUnit* FindBestBuilderForLocation(const FVector& TargetLocation) const;
    void AppendStopCommand(TArray<FRTSCommandButton>& OutCommands, int32& SlotIndex) const;
    void AppendProductionCommands(TArray<FRTSCommandButton>& OutCommands, int32& SlotIndex) const;
    void AppendWorkerBuildCommands(TArray<FRTSCommandButton>& OutCommands, int32& SlotIndex) const;
    void PopulateUnitCommandMetadata(FRTSCommandButton& Command, URTSUnitData* UnitData) const;
    void PopulateBuildingCommandMetadata(FRTSCommandButton& Command, URTSBuildingData* BuildingData) const;
    bool CanBuildingTrainUnit(const ARTSBuilding* Building, const URTSUnitData* UnitData) const;
    bool IsValidControlGroupIndex(int32 GroupIndex) const;
    bool HasStoppableSelection() const;
    void PruneControlGroup(FRTSControlGroup& Group) const;
    bool GetActorGroupCenter(const TArray<TObjectPtr<AActor>>& Actors, FVector& OutCenter) const;
    void MoveCameraToActorGroup(const TArray<TObjectPtr<AActor>>& Actors);
    void UpdateBuildingPreview();
    void CreatePreviewActor();
    void DestroyPreviewActor();
    void SetPreviewValidVisual(bool bValid);
    void CreateBuildGridPreviewActor();
    void DestroyBuildGridPreviewActor();

    bool StartWorkerBuildOrderOnServer(ARTSWorkerUnit* Worker, FName BuildingId, FRTSGridCoord OriginCoord);
    bool IsWorkerCloseEnoughToBuild(ARTSWorkerUnit* Worker, URTSBuildingData* BuildingData, FRTSGridCoord OriginCoord);
    URTSBuildingData* FindBuildableBuildingDataForWorker(ARTSWorkerUnit* Worker, FName BuildingId) const;
    void BuildOnServer(FName BuildingId, FRTSGridCoord OriginCoord);
    bool QueueProductionOnServer(ARTSBuilding* Building, FName UnitId);
    void IssueMoveCommandOnServer(const TArray<ARTSUnitBase*>& Units, const FVector& TargetLocation);
    void IssueAttackTargetCommandOnServer(const TArray<ARTSUnitBase*>& Units, AActor* TargetActor);
    void IssueAttackMoveCommandOnServer(const TArray<ARTSUnitBase*>& Units, const FVector& TargetLocation);
    void StopCommandsOnServer(const TArray<ARTSUnitBase*>& Units, const TArray<ARTSBuilding*>& Buildings);
    void IssueFlyingBuildingMoveCommandOnServer(const TArray<ARTSTerranBuilding*>& Buildings, const FVector& TargetLocation);
    void IssueGatherCommandOnServer(const TArray<ARTSWorkerUnit*>& Workers, ARTSResourceNode* ResourceNode);
    void IssueRallyPointCommandOnServer(const TArray<ARTSBuilding*>& Buildings, const FVector& TargetLocation);
    bool IsAttackCommandModifierDown() const;
    bool IsEnemyActorForLocalPlayer(AActor* Actor) const;
    void ShowCommandFeedback(const FVector& WorldLocation, bool bTargetActorFeedback, bool bAttackCommandFeedback) const;
    ARTSBuilding* FindFirstOwnedSelectedBuilding() const;
    URTSBuildingData* FindBuildingDataById(FName BuildingId) const;
    URTSUnitData* FindUnitDataById(FName UnitId) const;

    bool bIsDraggingSelection = false;
    FVector2D SelectionDragStart = FVector2D::ZeroVector;
    FVector2D SelectionDragEnd = FVector2D::ZeroVector;
};