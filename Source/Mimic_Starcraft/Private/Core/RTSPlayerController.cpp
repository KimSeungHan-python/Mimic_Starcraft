#include "Core/RTSPlayerController.h"
#include "Grid/RTSGridManager.h"
#include "Buildings/RTSBuilding.h"
#include "Data/RTSBuildingData.h"
#include "Data/RTSUnitData.h"
#include "Core/RTSPlayerState.h"
#include "Interfaces/RTSSelectableInterface.h"
#include "Resources/RTSResourceNode.h"
#include "Units/RTSUnitBase.h"
#include "Units/RTSWorkerUnit.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Buildings/RTSBuildGridPreview.h"
#include "InputCoreTypes.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

ARTSPlayerController::ARTSPlayerController()
{
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
}

void ARTSPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

    SetInputMode(InputMode);

    if (!GridManager)
    {
        GridManager = ResolveGridManager();
    }

    AddPlayerMappingContext();
}

void ARTSPlayerController::AddPlayerMappingContext()
{
    if (!PlayerMappingContext)
    {
        return;
    }

    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (!LocalPlayer)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

    if (!Subsystem)
    {
        return;
    }

    Subsystem->AddMappingContext(PlayerMappingContext, PlayerMappingPriority);
}

void ARTSPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (bIsInBuildMode)
    {
        UpdateBuildingPreview();
    }

    if (bIsDraggingSelection)
    {
        UpdateSelectionDrag();
    }
}

void ARTSPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);

    if (!EnhancedInputComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("RTSPlayerController: EnhancedInputComponent is null."));
        return;
    }

    if (ConfirmBuildAction)
    {
        EnhancedInputComponent->BindAction(
            ConfirmBuildAction,
            ETriggerEvent::Started,
            this,
            &ARTSPlayerController::HandleConfirmBuild
        );
    }

    if (CancelBuildAction)
    {
        EnhancedInputComponent->BindAction(
            CancelBuildAction,
            ETriggerEvent::Started,
            this,
            &ARTSPlayerController::HandleCancelBuild
        );
    }

    if (SelectAction)
    {
        EnhancedInputComponent->BindAction(
            SelectAction,
            ETriggerEvent::Started,
            this,
            &ARTSPlayerController::HandleSelectStarted
        );

        EnhancedInputComponent->BindAction(
            SelectAction,
            ETriggerEvent::Completed,
            this,
            &ARTSPlayerController::HandleSelectCompleted
        );

        EnhancedInputComponent->BindAction(
            SelectAction,
            ETriggerEvent::Canceled,
            this,
            &ARTSPlayerController::HandleSelectCompleted
        );
    }

    if (CommandAction)
    {
        EnhancedInputComponent->BindAction(
            CommandAction,
            ETriggerEvent::Started,
            this,
            &ARTSPlayerController::HandleCommand
        );
    }
}

void ARTSPlayerController::HandleConfirmBuild(const FInputActionValue& Value)
{
    ConfirmBuild();
}

void ARTSPlayerController::HandleCancelBuild(const FInputActionValue& Value)
{
    CancelBuildMode();
}

void ARTSPlayerController::HandleSelectStarted(const FInputActionValue& Value)
{
    BeginSelection();
}

void ARTSPlayerController::HandleSelectCompleted(const FInputActionValue& Value)
{
    EndSelection();
}

void ARTSPlayerController::HandleCommand(const FInputActionValue& Value)
{
    IssueSmartCommand();
}

void ARTSPlayerController::StartBuildMode(URTSBuildingData* BuildingData)
{
    if (!BuildingData)
    {
        return;
    }

    if (!GridManager)
    {
        GridManager = ResolveGridManager();
    }

    if (!GridManager)
    {
        return;
    }

    SelectedBuildingData = BuildingData;
    bIsInBuildMode = true;

    CreatePreviewActor();
    CreateBuildGridPreviewActor();
}

void ARTSPlayerController::CancelBuildMode()
{
    bIsInBuildMode = false;
    SelectedBuildingData = nullptr;
    bCurrentPlacementValid = false;
    bHasValidPreviewTransform = false;

    DestroyPreviewActor();
    DestroyBuildGridPreviewActor();
}

bool ARTSPlayerController::GetMouseWorldLocation(FVector& OutLocation) const
{
    FHitResult HitResult;

    const bool bHit = GetHitResultUnderCursor(
        GroundTraceChannel,
        false,
        HitResult
    );

    if (!bHit)
    {
        return false;
    }

    OutLocation = HitResult.Location;
    return true;
}

void ARTSPlayerController::UpdateBuildingPreview()
{
    if (!GridManager || !SelectedBuildingData)
    {
        bHasValidPreviewTransform = false;
        return;
    }

    FVector MouseWorldLocation;

    if (!GetMouseWorldLocation(MouseWorldLocation))
    {
        bCurrentPlacementValid = false;
        bHasValidPreviewTransform = false;

        SetPreviewValidVisual(false);

        if (BuildGridPreviewActor)
        {
            BuildGridPreviewActor->HidePreview();
        }

        return;
    }

    const FRTSGridCoord MouseCoord = GridManager->WorldToGrid(MouseWorldLocation);

    //CurrentPreviewCoord = MouseCoord;

    // 만약 마우스 위치를 건물 중앙으로 쓰고 싶으면 위 코드 대신 이걸 사용
    CurrentPreviewCoord.X = MouseCoord.X - SelectedBuildingData->GridWidth / 2;
    CurrentPreviewCoord.Y = MouseCoord.Y - SelectedBuildingData->GridHeight / 2;

    //bCurrentPlacementValid = GridManager->CanPlaceBuilding(
    //    CurrentPreviewCoord,
    //    SelectedBuildingData->GridWidth,
    //    SelectedBuildingData->GridHeight
    //);
    //수정됨
    bCurrentPlacementValid = GridManager->CanPlaceBuildingByData(
        CurrentPreviewCoord,
        SelectedBuildingData
    );

    FVector BuildingCenter;

    GridManager->GetBuildingCenterLocationOnGround(
        CurrentPreviewCoord,
        SelectedBuildingData->GridWidth,
        SelectedBuildingData->GridHeight,
        BuildingCenter
    );

    if (PreviewBuildingActor)
    {
        PreviewBuildingActor->SetActorLocation(BuildingCenter);
        PreviewBuildingActor->SetActorRotation(FRotator::ZeroRotator);

        LastPreviewTransform = PreviewBuildingActor->GetActorTransform();
        bHasValidPreviewTransform = true;
    }
    else
    {
        bHasValidPreviewTransform = false;
    }

    if (BuildGridPreviewActor)
    {
        BuildGridPreviewActor->UpdateFootprint(
            GridManager,
            CurrentPreviewCoord,
            SelectedBuildingData->GridWidth,
            SelectedBuildingData->GridHeight,
            bCurrentPlacementValid
        );
    }

    SetPreviewValidVisual(bCurrentPlacementValid);
}

void ARTSPlayerController::ConfirmBuild()
{
    if (!bIsInBuildMode)
    {
        return;
    }

    if (!GridManager || !SelectedBuildingData)
    {
        return;
    }

    if (!bCurrentPlacementValid)
    {
        return;
    }

    if (!bHasValidPreviewTransform)
    {
        return;
    }

    if (ARTSPlayerState* PS = GetPlayerState<ARTSPlayerState>())
    {
        if (!PS->CanAfford(SelectedBuildingData->MineralCost, SelectedBuildingData->VespeneCost))
        {
            return;
        }
    }

    const FName BuildingId = SelectedBuildingData->BuildingId;
    const FRTSGridCoord OriginCoord = CurrentPreviewCoord;

    if (HasAuthority())
    {
        BuildOnServer(BuildingId, OriginCoord);
    }
    else
    {
        ServerConfirmBuild(BuildingId, OriginCoord);
    }

    CancelBuildMode();
}

void ARTSPlayerController::ServerConfirmBuild_Implementation(FName BuildingId, FRTSGridCoord OriginCoord)
{
    BuildOnServer(BuildingId, OriginCoord);
}

void ARTSPlayerController::BeginSelection()
{
    if (bIsInBuildMode)
    {
        return;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        return;
    }

    bIsDraggingSelection = true;
    SelectionDragStart = FVector2D(MouseX, MouseY);
    SelectionDragEnd = SelectionDragStart;
}

void ARTSPlayerController::UpdateSelectionDrag()
{
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (GetMousePosition(MouseX, MouseY))
    {
        SelectionDragEnd = FVector2D(MouseX, MouseY);
    }
}

void ARTSPlayerController::EndSelection()
{
    if (!bIsDraggingSelection)
    {
        return;
    }

    UpdateSelectionDrag();
    bIsDraggingSelection = false;

    const bool bAppendSelection = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
    const float DragDistance = FVector2D::Distance(SelectionDragStart, SelectionDragEnd);

    if (DragDistance <= DragSelectThresholdPixels)
    {
        SelectSingleActorUnderCursor(bAppendSelection);
        return;
    }

    SelectActorsInScreenRect(SelectionDragStart, SelectionDragEnd, bAppendSelection);
}

void ARTSPlayerController::ClearSelection()
{
    for (AActor* SelectedActor : SelectedActors)
    {
        if (SelectedActor && SelectedActor->GetClass()->ImplementsInterface(URTSSelectableInterface::StaticClass()))
        {
            IRTSSelectableInterface::Execute_SetSelectionState(SelectedActor, false);
        }
    }

    SelectedActors.Reset();
}

void ARTSPlayerController::SelectActor(AActor* Actor, bool bAppendSelection)
{
    if (!IsActorSelectable(Actor))
    {
        if (!bAppendSelection)
        {
            ClearSelection();
        }
        return;
    }

    if (!bAppendSelection)
    {
        ClearSelection();
    }

    SelectedActors.AddUnique(Actor);
    IRTSSelectableInterface::Execute_SetSelectionState(Actor, true);
}

void ARTSPlayerController::SelectSingleActorUnderCursor(bool bAppendSelection)
{
    FHitResult HitResult;
    const bool bHit = GetHitResultUnderCursor(
        SelectionTraceChannel,
        false,
        HitResult
    );

    AActor* HitActor = bHit ? HitResult.GetActor() : nullptr;
    const bool bCtrlSelection = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);

    if (bCtrlSelection && IsActorSelectable(HitActor) && IsOwnedByLocalPlayer(HitActor))
    {
        SelectVisibleActorsOfSameClass(HitActor, bAppendSelection);
        return;
    }

    SelectActor(HitActor, bAppendSelection);
}

void ARTSPlayerController::SelectActorsInScreenRect(
    const FVector2D& StartScreen,
    const FVector2D& EndScreen,
    bool bAppendSelection
)
{
    const float MinX = FMath::Min(StartScreen.X, EndScreen.X);
    const float MaxX = FMath::Max(StartScreen.X, EndScreen.X);
    const float MinY = FMath::Min(StartScreen.Y, EndScreen.Y);
    const float MaxY = FMath::Max(StartScreen.Y, EndScreen.Y);

    TArray<AActor*> UnitHits;
    TArray<AActor*> BuildingHits;

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Candidate = *It;
        if (!IsActorSelectable(Candidate) || !IsOwnedByLocalPlayer(Candidate))
        {
            continue;
        }

        FVector Origin;
        FVector Extent;
        Candidate->GetActorBounds(false, Origin, Extent);

        FVector2D ScreenPosition;
        if (!ProjectWorldLocationToScreen(Origin, ScreenPosition))
        {
            continue;
        }

        const bool bInsideRect = ScreenPosition.X >= MinX
            && ScreenPosition.X <= MaxX
            && ScreenPosition.Y >= MinY
            && ScreenPosition.Y <= MaxY;

        if (!bInsideRect)
        {
            continue;
        }

        if (Candidate->IsA<ARTSUnitBase>())
        {
            UnitHits.Add(Candidate);
        }
        else if (Candidate->IsA<ARTSBuilding>())
        {
            BuildingHits.Add(Candidate);
        }
    }

    const TArray<AActor*>& TargetHits = UnitHits.Num() > 0 ? UnitHits : BuildingHits;

    if (!bAppendSelection)
    {
        ClearSelection();
    }

    for (AActor* Actor : TargetHits)
    {
        SelectActor(Actor, true);
    }
}

bool ARTSPlayerController::IsActorSelectable(AActor* Actor) const
{
    return Actor
        && !Actor->ActorHasTag(TEXT("BuildPreview"))
        && Actor->GetClass()->ImplementsInterface(URTSSelectableInterface::StaticClass())
        && IRTSSelectableInterface::Execute_CanBeSelectedBy(Actor, const_cast<ARTSPlayerController*>(this));
}

bool ARTSPlayerController::IsOwnedByLocalPlayer(AActor* Actor) const
{
    ARTSPlayerState* LocalPlayerState = GetPlayerState<ARTSPlayerState>();
    return Actor
        && LocalPlayerState
        && Actor->GetClass()->ImplementsInterface(URTSSelectableInterface::StaticClass())
        && IRTSSelectableInterface::Execute_IsOwnedByPlayerState(Actor, LocalPlayerState);
}

bool ARTSPlayerController::IsActorVisibleOnScreen(AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    int32 ViewportX = 0;
    int32 ViewportY = 0;
    GetViewportSize(ViewportX, ViewportY);

    if (ViewportX <= 0 || ViewportY <= 0)
    {
        return false;
    }

    FVector Origin;
    FVector Extent;
    Actor->GetActorBounds(false, Origin, Extent);

    FVector2D ScreenPosition;
    if (!ProjectWorldLocationToScreen(Origin, ScreenPosition))
    {
        return false;
    }

    return ScreenPosition.X >= 0.0f
        && ScreenPosition.X <= ViewportX
        && ScreenPosition.Y >= 0.0f
        && ScreenPosition.Y <= ViewportY;
}

void ARTSPlayerController::SelectVisibleActorsOfSameClass(AActor* SourceActor, bool bAppendSelection)
{
    if (!IsActorSelectable(SourceActor) || !IsOwnedByLocalPlayer(SourceActor))
    {
        SelectActor(SourceActor, bAppendSelection);
        return;
    }

    if (!bAppendSelection)
    {
        ClearSelection();
    }

    const UClass* TargetClass = SourceActor->GetClass();
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Candidate = *It;
        if (!Candidate || Candidate->GetClass() != TargetClass)
        {
            continue;
        }

        if (!IsActorSelectable(Candidate) || !IsOwnedByLocalPlayer(Candidate) || !IsActorVisibleOnScreen(Candidate))
        {
            continue;
        }

        SelectedActors.AddUnique(Candidate);
        IRTSSelectableInterface::Execute_SetSelectionState(Candidate, true);
    }
}
TArray<ARTSUnitBase*> ARTSPlayerController::GetOwnedSelectedUnits() const
{
    TArray<ARTSUnitBase*> Units;
    ARTSPlayerState* LocalPlayerState = GetPlayerState<ARTSPlayerState>();
    if (!LocalPlayerState)
    {
        return Units;
    }

    for (AActor* SelectedActor : SelectedActors)
    {
        ARTSUnitBase* Unit = Cast<ARTSUnitBase>(SelectedActor);
        if (!Unit)
        {
            continue;
        }

        if (IRTSSelectableInterface::Execute_IsOwnedByPlayerState(Unit, LocalPlayerState))
        {
            Units.Add(Unit);
        }
    }

    return Units;
}

TArray<ARTSWorkerUnit*> ARTSPlayerController::GetOwnedSelectedWorkers() const
{
    TArray<ARTSWorkerUnit*> Workers;

    for (ARTSUnitBase* Unit : GetOwnedSelectedUnits())
    {
        if (ARTSWorkerUnit* Worker = Cast<ARTSWorkerUnit>(Unit))
        {
            Workers.Add(Worker);
        }
    }

    return Workers;
}

void ARTSPlayerController::IssueSmartCommand()
{
    if (bIsInBuildMode)
    {
        return;
    }

    FHitResult HitResult;
    const bool bHit = GetHitResultUnderCursor(
        SelectionTraceChannel,
        false,
        HitResult
    );

    if (!bHit)
    {
        return;
    }

    if (ARTSResourceNode* ResourceNode = Cast<ARTSResourceNode>(HitResult.GetActor()))
    {
        const TArray<ARTSWorkerUnit*> Workers = GetOwnedSelectedWorkers();
        if (Workers.Num() == 0)
        {
            return;
        }

        if (HasAuthority())
        {
            IssueGatherCommandOnServer(Workers, ResourceNode);
        }
        else
        {
            ServerIssueGatherCommand(Workers, ResourceNode);
        }
        return;
    }

    const TArray<ARTSUnitBase*> Units = GetOwnedSelectedUnits();
    if (Units.Num() == 0)
    {
        return;
    }

    if (HasAuthority())
    {
        IssueMoveCommandOnServer(Units, HitResult.Location);
    }
    else
    {
        ServerIssueMoveCommand(Units, HitResult.Location);
    }
}

void ARTSPlayerController::ServerIssueMoveCommand_Implementation(
    const TArray<ARTSUnitBase*>& Units,
    FVector TargetLocation
)
{
    IssueMoveCommandOnServer(Units, TargetLocation);
}

void ARTSPlayerController::ServerIssueGatherCommand_Implementation(
    const TArray<ARTSWorkerUnit*>& Workers,
    ARTSResourceNode* ResourceNode
)
{
    IssueGatherCommandOnServer(Workers, ResourceNode);
}

void ARTSPlayerController::IssueMoveCommandOnServer(
    const TArray<ARTSUnitBase*>& Units,
    const FVector& TargetLocation
)
{
    if (!HasAuthority())
    {
        return;
    }

    for (int32 Index = 0; Index < Units.Num(); ++Index)
    {
        ARTSUnitBase* Unit = Units[Index];
        if (!Unit || !Unit->CanReceiveCommandsFrom(this))
        {
            continue;
        }

        if (ARTSWorkerUnit* Worker = Cast<ARTSWorkerUnit>(Unit))
        {
            Worker->StopWorkerCommand();
        }

        const int32 Row = Index / 3;
        const int32 Col = Index % 3;
        const FVector FormationOffset((Col - 1) * 85.0f, Row * 85.0f, 0.0f);
        Unit->IssueMoveCommand(TargetLocation + FormationOffset);
    }
}

void ARTSPlayerController::IssueGatherCommandOnServer(
    const TArray<ARTSWorkerUnit*>& Workers,
    ARTSResourceNode* ResourceNode
)
{
    if (!HasAuthority() || !ResourceNode || !ResourceNode->HasResources())
    {
        return;
    }

    for (ARTSWorkerUnit* Worker : Workers)
    {
        if (!Worker || !Worker->CanReceiveCommandsFrom(this))
        {
            continue;
        }

        Worker->GatherFromResource(ResourceNode);
    }
}
bool ARTSPlayerController::QueueProductionForSelectedBuilding(URTSUnitData* UnitData)
{
    if (!UnitData)
    {
        return false;
    }

    ARTSBuilding* Building = FindFirstOwnedSelectedBuilding();
    if (!Building)
    {
        return false;
    }

    if (HasAuthority())
    {
        return QueueProductionOnServer(Building, UnitData->UnitId);
    }

    ServerQueueProduction(Building, UnitData->UnitId);
    return true;
}

void ARTSPlayerController::ServerQueueProduction_Implementation(ARTSBuilding* Building, FName UnitId)
{
    QueueProductionOnServer(Building, UnitId);
}

bool ARTSPlayerController::QueueProductionOnServer(ARTSBuilding* Building, FName UnitId)
{
    if (!HasAuthority())
    {
        return false;
    }

    URTSUnitData* UnitData = FindUnitDataById(UnitId);
    if (!Building || !UnitData || !Building->CanReceiveCommandsFrom(this))
    {
        return false;
    }

    return Building->QueueUnitProduction(UnitData);
}

ARTSBuilding* ARTSPlayerController::FindFirstOwnedSelectedBuilding() const
{
    ARTSPlayerState* LocalPlayerState = GetPlayerState<ARTSPlayerState>();
    if (!LocalPlayerState)
    {
        return nullptr;
    }

    for (AActor* SelectedActor : SelectedActors)
    {
        ARTSBuilding* Building = Cast<ARTSBuilding>(SelectedActor);
        if (!Building)
        {
            continue;
        }

        if (IRTSSelectableInterface::Execute_IsOwnedByPlayerState(Building, LocalPlayerState))
        {
            return Building;
        }
    }

    return nullptr;
}
void ARTSPlayerController::BuildOnServer(FName BuildingId, FRTSGridCoord OriginCoord)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!GridManager)
    {
        GridManager = ResolveGridManager();
    }

    if (!GridManager)
    {
        UE_LOG(LogTemp, Error, TEXT("BuildOnServer failed: GridManager is null"));
        return;
    }

    URTSBuildingData* BuildingData = FindBuildingDataById(BuildingId);

    if (!BuildingData)
    {
        UE_LOG(LogTemp, Error, TEXT("BuildOnServer failed: BuildingData not found. BuildingId=%s"), *BuildingId.ToString());
        return;
    }

    if (!GridManager->CanPlaceBuildingByData(OriginCoord, BuildingData))
    {
        UE_LOG(LogTemp, Warning, TEXT("BuildOnServer rejected: invalid placement"));
        return;
    }

    TSubclassOf<ARTSBuilding> BuildingClass = BuildingData->BuildingClass;

    if (!BuildingClass)
    {
        BuildingClass = DefaultBuildingClass;
    }

    if (!BuildingClass)
    {
        UE_LOG(LogTemp, Error, TEXT("BuildOnServer failed: BuildingClass is null"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("BuildOnServer failed: World is null"));
        return;
    }

    ARTSPlayerState* OwningPS = GetPlayerState<ARTSPlayerState>();
    if (!OwningPS)
    {
        UE_LOG(LogTemp, Error, TEXT("BuildOnServer failed: PlayerState is null"));
        return;
    }

    FVector BuildingCenter;
    GridManager->GetBuildingCenterLocationOnGround(
        OriginCoord,
        BuildingData->GridWidth,
        BuildingData->GridHeight,
        BuildingCenter
    );

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(BuildingCenter);
    SpawnTransform.SetRotation(FRotator::ZeroRotator.Quaternion());
    SpawnTransform.SetScale3D(FVector::OneVector);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (!OwningPS->TrySpendResources(BuildingData->MineralCost, BuildingData->VespeneCost))
    {
        UE_LOG(LogTemp, Warning, TEXT("BuildOnServer rejected: not enough resources"));
        return;
    }

    ARTSBuilding* NewBuilding = World->SpawnActor<ARTSBuilding>(
        BuildingClass,
        SpawnTransform,
        SpawnParams
    );

    if (!NewBuilding)
    {
        OwningPS->AddResources(BuildingData->MineralCost, BuildingData->VespeneCost);
        UE_LOG(LogTemp, Error, TEXT("BuildOnServer failed: SpawnActor returned null"));
        return;
    }

    NewBuilding->OwningPlayerState = OwningPS;
    NewBuilding->SetTeamInfo(OwningPS->TeamNumber, OwningPS->TeamColor);

    NewBuilding->InitializeBuilding(
        BuildingData,
        OriginCoord,
        BuildingData->GridWidth,
        BuildingData->GridHeight,
        GridManager->CellSize,
        GridManager
    );

    NewBuilding->SetPreviewBuildingMode(false);
    NewBuilding->BeginConstruction(BuildingData->BuildTime);

    UE_LOG(LogTemp, Log, TEXT("Server spawned building: %s"), *NewBuilding->GetName());
}


URTSUnitData* ARTSPlayerController::FindUnitDataById(FName UnitId) const
{
    for (URTSUnitData* Data : UnitDataList)
    {
        if (!Data)
        {
            continue;
        }

        if (Data->UnitId == UnitId)
        {
            return Data;
        }
    }

    return nullptr;
}

URTSBuildingData* ARTSPlayerController::FindBuildingDataById(FName BuildingId) const
{
    for (URTSBuildingData* Data : BuildingDataList)
    {
        if (!Data)
        {
            continue;
        }

        if (Data->BuildingId == BuildingId)
        {
            return Data;
        }
    }

    // Listen Server Host에서 테스트할 때를 위한 보조 fallback
    if (SelectedBuildingData && SelectedBuildingData->BuildingId == BuildingId)
    {
        return SelectedBuildingData;
    }

    return nullptr;
}

void ARTSPlayerController::Client_SetStartCamera_Implementation(const FTransform& CameraTransform)
{
    APawn* ControlledPawn = GetPawn();
    if (ControlledPawn)
    {
        ControlledPawn->SetActorTransform(CameraTransform);
        SetControlRotation(CameraTransform.GetRotation().Rotator());
    }
}

void ARTSPlayerController::CreatePreviewActor()
{
    DestroyPreviewActor();

    if (!SelectedBuildingData || !GridManager)
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    TSubclassOf<ARTSBuilding> PreviewClass = SelectedBuildingData->BuildingClass;

    if (!PreviewClass)
    {
        PreviewClass = DefaultBuildingClass;
    }

    if (!PreviewClass)
    {
        UE_LOG(LogTemp, Error, TEXT("CreatePreviewActor failed: No PreviewClass"));
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    PreviewBuildingActor = World->SpawnActor<ARTSBuilding>(
        PreviewClass,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (!PreviewBuildingActor)
    {
        UE_LOG(LogTemp, Error, TEXT("CreatePreviewActor failed: Spawn failed"));
        return;
    }

    PreviewBuildingActor->SetReplicates(false);
    PreviewBuildingActor->SetReplicateMovement(false);
    PreviewBuildingActor->InitializeBuilding(
        SelectedBuildingData,
        FRTSGridCoord(0, 0),
        SelectedBuildingData->GridWidth,
        SelectedBuildingData->GridHeight,
        GridManager->CellSize,
        GridManager
    );

    // 실제 건물과 같은 크기 보정 사용
    PreviewBuildingActor->FitMeshToGridFootprint(GridManager->CellSize);

    // 프리뷰는 충돌, 네비 영향 제거
    PreviewBuildingActor->SetPreviewBuildingMode(true);

    // 프리뷰가 실제 게임 오브젝트처럼 선택/타겟팅되지 않게 태그 추가<- 후에 추가
    PreviewBuildingActor->Tags.Add(TEXT("BuildPreview"));

    SetPreviewValidVisual(false);
}

void ARTSPlayerController::DestroyPreviewActor()
{
    if (PreviewBuildingActor)
    {
        PreviewBuildingActor->Destroy();
        PreviewBuildingActor = nullptr;
    }
}

void ARTSPlayerController::SetPreviewValidVisual(bool bValid)
{
    if (!PreviewBuildingActor)
    {
        return;
    }

    UMaterialInterface* TargetMaterial = bValid
        ? ValidPreviewMaterial
        : InvalidPreviewMaterial;

    if (!TargetMaterial)
    {
        return;
    }

    TArray<UStaticMeshComponent*> MeshComponents;
    PreviewBuildingActor->GetComponents<UStaticMeshComponent>(MeshComponents);

    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (!MeshComp)
        {
            continue;
        }

        const int32 MaterialCount = MeshComp->GetNumMaterials();

        for (int32 i = 0; i < MaterialCount; ++i)
        {
            MeshComp->SetMaterial(i, TargetMaterial);
        }
    }
}

void ARTSPlayerController::CreateBuildGridPreviewActor()
{
    if (BuildGridPreviewActor)
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    TSubclassOf<ARTSBuildGridPreview> ClassToSpawn = BuildGridPreviewClass;

    if (!ClassToSpawn)
    {
        ClassToSpawn = ARTSBuildGridPreview::StaticClass();
    }

    BuildGridPreviewActor = World->SpawnActor<ARTSBuildGridPreview>(
        ClassToSpawn,
        FVector::ZeroVector,
        FRotator::ZeroRotator
    );
}

void ARTSPlayerController::DestroyBuildGridPreviewActor()
{
    if (BuildGridPreviewActor)
    {
        BuildGridPreviewActor->Destroy();
        BuildGridPreviewActor = nullptr;
    }
}

ARTSGridManager* ARTSPlayerController::ResolveGridManager()
{
    if (!GetWorld())
    {
        return nullptr;
    }

    for (TActorIterator<ARTSGridManager> It(GetWorld()); It; ++It)
    {
        return *It;
    }

    return nullptr;
}
