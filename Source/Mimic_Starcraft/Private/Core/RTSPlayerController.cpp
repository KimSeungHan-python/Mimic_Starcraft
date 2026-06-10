#include "Core/RTSPlayerController.h"
#include "Grid/RTSGridManager.h"
#include "Buildings/RTSBuilding.h"
#include "Data/RTSBuildingData.h"
#include "Data/RTSUnitData.h"
#include "Core/RTSPlayerState.h"
#include "Interfaces/RTSSelectableInterface.h"
#include "Resources/RTSResourceNode.h"
#include "Races/Terran/RTSTerranBuilding.h"
#include "Units/RTSUnitBase.h"
#include "Units/RTSWorkerUnit.h"
#include "Vision/RTSVisionManager.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/RTSProductionQueueComponent.h"
#include "Components/RTSAttackComponent.h"
#include "Components/RTSHealthComponent.h"
#include "Components/RTSWorkerBuildComponent.h"
#include "Buildings/RTSBuildGridPreview.h"
#include "InputCoreTypes.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"

ARTSPlayerController::ARTSPlayerController()
{
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
    VisionManagerClass = ARTSVisionManager::StaticClass();

    DefaultCommandHotkeys = {
        EKeys::Q, EKeys::W, EKeys::E, EKeys::R, EKeys::T,
        EKeys::A, EKeys::S, EKeys::D, EKeys::F, EKeys::G,
        EKeys::Z, EKeys::X, EKeys::C, EKeys::V, EKeys::B
    };
}

void ARTSPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    ConfigureMouseInputMode();

    if (!GridManager)
    {
        GridManager = ResolveGridManager();
    }

    ResolveVisionManager();
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

void ARTSPlayerController::ConfigureMouseInputMode()
{
    if (!IsLocalController())
    {
        return;
    }

    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(
        bLockMouseToViewport ? EMouseLockMode::LockAlways : EMouseLockMode::DoNotLock
    );

    SetInputMode(InputMode);
}

void ARTSPlayerController::ClampMouseToViewport()
{
    if (!IsLocalController() || !bLockMouseToViewport)
    {
        return;
    }

    float RawMouseX = 0.0f;
    float RawMouseY = 0.0f;
    if (!GetMousePosition(RawMouseX, RawMouseY))
    {
        return;
    }

    FVector2D ClampedMousePosition;
    if (!GetClampedMousePosition(ClampedMousePosition))
    {
        return;
    }

    if (!FMath::IsNearlyEqual(RawMouseX, ClampedMousePosition.X, 0.5f)
        || !FMath::IsNearlyEqual(RawMouseY, ClampedMousePosition.Y, 0.5f))
    {
        SetMouseLocation(
            FMath::RoundToInt(ClampedMousePosition.X),
            FMath::RoundToInt(ClampedMousePosition.Y)
        );
    }
}

bool ARTSPlayerController::GetClampedMousePosition(FVector2D& OutMousePosition) const
{
    int32 ViewportX = 0;
    int32 ViewportY = 0;
    GetViewportSize(ViewportX, ViewportY);

    if (ViewportX <= 0 || ViewportY <= 0)
    {
        return false;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        return false;
    }

    const float ViewportWidth = static_cast<float>(ViewportX);
    const float ViewportHeight = static_cast<float>(ViewportY);
    const float MaxSafePadding = FMath::Max(0.0f, FMath::Min(ViewportWidth, ViewportHeight) * 0.5f - 1.0f);
    const float Padding = FMath::Clamp(MouseViewportClampPadding, 0.0f, MaxSafePadding);
    const float MinX = Padding;
    const float MinY = Padding;
    const float MaxX = FMath::Max(MinX, ViewportWidth - 1.0f - Padding);
    const float MaxY = FMath::Max(MinY, ViewportHeight - 1.0f - Padding);

    OutMousePosition = FVector2D(
        FMath::Clamp(MouseX, MinX, MaxX),
        FMath::Clamp(MouseY, MinY, MaxY)
    );
    return true;
}
void ARTSPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    ClampMouseToViewport();

    if (bIsInBuildMode)
    {
        UpdateBuildingPreview();
    }

    if (bIsDraggingSelection)
    {
        UpdateSelectionDrag();
    }

    HandleKeyboardShortcuts();
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

void ARTSPlayerController::HandleKeyboardShortcuts()
{
    if (bIsInBuildMode)
    {
        return;
    }

    if (bEnableDefaultControlGroupHotkeys)
    {
        const FKey NumberKeys[10] = {
            EKeys::Zero,
            EKeys::One,
            EKeys::Two,
            EKeys::Three,
            EKeys::Four,
            EKeys::Five,
            EKeys::Six,
            EKeys::Seven,
            EKeys::Eight,
            EKeys::Nine
        };

        for (int32 GroupIndex = 0; GroupIndex < 10; ++GroupIndex)
        {
            if (WasInputKeyJustPressed(NumberKeys[GroupIndex]))
            {
                HandleControlGroupInput(GroupIndex);
                return;
            }
        }
    }

    if (!bEnableDefaultCommandHotkeys)
    {
        return;
    }

    if (WasInputKeyJustPressed(EKeys::Escape) && !ActiveCommandPage.IsNone())
    {
        ClearCommandPage();
        return;
    }

    const bool bCtrlDown = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
    const bool bAltDown = IsInputKeyDown(EKeys::LeftAlt) || IsInputKeyDown(EKeys::RightAlt);
    if (bCtrlDown || bAltDown)
    {
        return;
    }

    for (const FKey& Hotkey : DefaultCommandHotkeys)
    {
        if (WasInputKeyJustPressed(Hotkey) && ExecuteCommandHotkey(Hotkey))
        {
            return;
        }
    }
}
ARTSVisionManager* ARTSPlayerController::ResolveVisionManager()
{
    if (VisionManager)
    {
        return VisionManager;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<ARTSVisionManager> It(World); It; ++It)
    {
        VisionManager = *It;
        break;
    }

    if (!VisionManager && VisionManagerClass && IsLocalController())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        VisionManager = World->SpawnActor<ARTSVisionManager>(VisionManagerClass, FTransform::Identity, SpawnParams);
    }

    if (VisionManager && GridManager)
    {
        VisionManager->GridManager = GridManager;
    }

    return VisionManager;
}

bool ARTSPlayerController::IsActorVisibleToLocalPlayer(AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    if (!bUseVisionForEnemyVisibility || IsOwnedByLocalPlayer(Actor))
    {
        return true;
    }

    const ARTSPlayerState* LocalPlayerState = GetPlayerState<ARTSPlayerState>();
    if (!LocalPlayerState || !Actor->GetClass()->ImplementsInterface(URTSSelectableInterface::StaticClass()))
    {
        return true;
    }

    const int32 TargetTeam = IRTSSelectableInterface::Execute_GetSelectableTeamNumber(Actor);
    if (TargetTeam < 0 || TargetTeam == LocalPlayerState->TeamNumber)
    {
        return true;
    }

    ARTSVisionManager* EffectiveVisionManager = VisionManager
        ? VisionManager.Get()
        : const_cast<ARTSPlayerController*>(this)->ResolveVisionManager();

    return !EffectiveVisionManager
        || EffectiveVisionManager->IsActorVisibleToTeam(Actor, LocalPlayerState->TeamNumber);
}

bool ARTSPlayerController::IsWorldLocationVisibleToLocalPlayer(const FVector& WorldLocation) const
{
    const ARTSPlayerState* LocalPlayerState = GetPlayerState<ARTSPlayerState>();
    if (!LocalPlayerState)
    {
        return true;
    }

    ARTSVisionManager* EffectiveVisionManager = VisionManager
        ? VisionManager.Get()
        : const_cast<ARTSPlayerController*>(this)->ResolveVisionManager();

    return !EffectiveVisionManager
        || EffectiveVisionManager->IsWorldLocationVisibleToTeam(WorldLocation, LocalPlayerState->TeamNumber);
}

bool ARTSPlayerController::IsWorldLocationExploredByLocalPlayer(const FVector& WorldLocation) const
{
    const ARTSPlayerState* LocalPlayerState = GetPlayerState<ARTSPlayerState>();
    if (!LocalPlayerState)
    {
        return true;
    }

    ARTSVisionManager* EffectiveVisionManager = VisionManager
        ? VisionManager.Get()
        : const_cast<ARTSPlayerController*>(this)->ResolveVisionManager();

    return !EffectiveVisionManager
        || EffectiveVisionManager->IsWorldLocationExploredByTeam(WorldLocation, LocalPlayerState->TeamNumber);
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
    FVector2D MousePosition;
    if (!GetClampedMousePosition(MousePosition))
    {
        return false;
    }

    FHitResult HitResult;
    const bool bHit = GetHitResultAtScreenPosition(
        MousePosition,
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
    ARTSWorkerUnit* Builder = FindBestBuilderForLocation(LastPreviewTransform.GetLocation());

    if (Builder)
    {
        if (HasAuthority())
        {
            StartWorkerBuildOrderOnServer(Builder, BuildingId, OriginCoord);
        }
        else
        {
            ServerStartWorkerBuild(Builder, BuildingId, OriginCoord);
        }

        CancelBuildMode();
        return;
    }

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

void ARTSPlayerController::ServerStartWorkerBuild_Implementation(
    ARTSWorkerUnit* Worker,
    FName BuildingId,
    FRTSGridCoord OriginCoord
)
{
    StartWorkerBuildOrderOnServer(Worker, BuildingId, OriginCoord);
}
void ARTSPlayerController::BeginSelection()
{
    if (bIsInBuildMode)
    {
        return;
    }

    FVector2D MousePosition;
    if (!GetClampedMousePosition(MousePosition))
    {
        return;
    }

    bIsDraggingSelection = true;
    SelectionDragStart = MousePosition;
    SelectionDragEnd = SelectionDragStart;
}

void ARTSPlayerController::UpdateSelectionDrag()
{
    FVector2D MousePosition;
    if (GetClampedMousePosition(MousePosition))
    {
        SelectionDragEnd = MousePosition;
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

void ARTSPlayerController::BroadcastSelectionChanged()
{
    ActiveCommandPage = NAME_None;
    OnSelectionChanged.Broadcast();
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
    BroadcastSelectionChanged();
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
    BroadcastSelectionChanged();
}

void ARTSPlayerController::SelectSingleActorUnderCursor(bool bAppendSelection)
{
    FVector2D MousePosition;
    if (!GetClampedMousePosition(MousePosition))
    {
        return;
    }

    FHitResult HitResult;
    const bool bHit = GetHitResultAtScreenPosition(
        MousePosition,
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
        && IRTSSelectableInterface::Execute_CanBeSelectedBy(Actor, const_cast<ARTSPlayerController*>(this))
        && IsActorVisibleToLocalPlayer(Actor);
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

    BroadcastSelectionChanged();
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

ARTSWorkerUnit* ARTSPlayerController::FindBestBuilderForLocation(const FVector& TargetLocation) const
{
    ARTSWorkerUnit* BestWorker = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    for (ARTSWorkerUnit* Worker : GetOwnedSelectedWorkers())
    {
        if (!Worker)
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(Worker->GetActorLocation(), TargetLocation);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestWorker = Worker;
        }
    }

    return BestWorker;
}
TArray<ARTSBuilding*> ARTSPlayerController::GetOwnedSelectedBuildings() const
{
    TArray<ARTSBuilding*> Buildings;
    ARTSPlayerState* LocalPlayerState = GetPlayerState<ARTSPlayerState>();
    if (!LocalPlayerState)
    {
        return Buildings;
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
            Buildings.Add(Building);
        }
    }

    return Buildings;
}
TArray<ARTSTerranBuilding*> ARTSPlayerController::GetOwnedSelectedFlyingTerranBuildings() const
{
    TArray<ARTSTerranBuilding*> FlyingBuildings;

    for (ARTSBuilding* Building : GetOwnedSelectedBuildings())
    {
        ARTSTerranBuilding* TerranBuilding = Cast<ARTSTerranBuilding>(Building);
        if (TerranBuilding && TerranBuilding->BuildingState == ERTSBuildingState::Flying)
        {
            FlyingBuildings.Add(TerranBuilding);
        }
    }

    return FlyingBuildings;
}

void ARTSPlayerController::PopulateUnitCommandMetadata(FRTSCommandButton& Command, URTSUnitData* UnitData) const
{
    if (!UnitData)
    {
        Command.bEnabled = false;
        Command.DisabledReason = FText::FromString(TEXT("Missing unit data"));
        return;
    }

    Command.Icon = UnitData->CommandIcon;
    Command.TooltipText = UnitData->CommandTooltip;
    Command.MineralCost = UnitData->MineralCost;
    Command.VespeneCost = UnitData->VespeneCost;
    Command.SupplyCost = UnitData->SupplyCost;
    Command.SortOrder = UnitData->CommandCardOrder;
    Command.bEnabled = true;
    Command.DisabledReason = FText::GetEmpty();

    ARTSPlayerState* LocalPlayerState = GetPlayerState<ARTSPlayerState>();
    if (!LocalPlayerState)
    {
        Command.bEnabled = false;
        Command.DisabledReason = FText::FromString(TEXT("Missing player state"));
        return;
    }

    if (!UnitData->UnitClass)
    {
        Command.bEnabled = false;
        Command.DisabledReason = FText::FromString(TEXT("Missing unit class"));
        return;
    }

    if (!LocalPlayerState->CanAfford(UnitData->MineralCost, UnitData->VespeneCost))
    {
        Command.bEnabled = false;
        Command.DisabledReason = FText::FromString(TEXT("Not enough resources"));
        return;
    }

    if (!LocalPlayerState->CanReserveSupply(UnitData->SupplyCost))
    {
        Command.bEnabled = false;
        Command.DisabledReason = FText::FromString(TEXT("Supply blocked"));
        return;
    }

    bool bHasAvailableProducer = false;
    for (ARTSBuilding* Building : GetOwnedSelectedBuildings())
    {
        if (!CanBuildingTrainUnit(Building, UnitData))
        {
            continue;
        }

        if (Building->ProductionQueueComponent
            && Building->ProductionQueueComponent->CanQueueUnit(UnitData, LocalPlayerState))
        {
            bHasAvailableProducer = true;
            break;
        }
    }

    if (!bHasAvailableProducer)
    {
        Command.bEnabled = false;
        Command.DisabledReason = FText::FromString(TEXT("Production queue unavailable"));
    }
}

void ARTSPlayerController::PopulateBuildingCommandMetadata(FRTSCommandButton& Command, URTSBuildingData* BuildingData) const
{
    if (!BuildingData)
    {
        Command.bEnabled = false;
        Command.DisabledReason = FText::FromString(TEXT("Missing building data"));
        return;
    }

    Command.Icon = BuildingData->CommandIcon;
    Command.TooltipText = BuildingData->CommandTooltip;
    Command.MineralCost = BuildingData->MineralCost;
    Command.VespeneCost = BuildingData->VespeneCost;
    Command.SupplyCost = 0;
    Command.SortOrder = BuildingData->CommandCardOrder;
    Command.bEnabled = true;
    Command.DisabledReason = FText::GetEmpty();

    ARTSPlayerState* LocalPlayerState = GetPlayerState<ARTSPlayerState>();
    if (!LocalPlayerState)
    {
        Command.bEnabled = false;
        Command.DisabledReason = FText::FromString(TEXT("Missing player state"));
        return;
    }

    if (!BuildingData->BuildingClass && !DefaultBuildingClass)
    {
        Command.bEnabled = false;
        Command.DisabledReason = FText::FromString(TEXT("Missing building class"));
        return;
    }

    if (!LocalPlayerState->CanAfford(BuildingData->MineralCost, BuildingData->VespeneCost))
    {
        Command.bEnabled = false;
        Command.DisabledReason = FText::FromString(TEXT("Not enough resources"));
        return;
    }

    if (GetOwnedSelectedWorkers().Num() == 0)
    {
        Command.bEnabled = false;
        Command.DisabledReason = FText::FromString(TEXT("No worker selected"));
    }
}
void ARTSPlayerController::GetAvailableCommandButtons(TArray<FRTSCommandButton>& OutCommands) const
{
    OutCommands.Reset();

    int32 SlotIndex = 0;

    if (!ActiveCommandPage.IsNone())
    {
        if (MaxCommandCardSlots <= 0 || OutCommands.Num() < MaxCommandCardSlots)
        {
            FRTSCommandButton BackCommand;
            BackCommand.SlotIndex = SlotIndex++;
            BackCommand.CommandId = TEXT("Back");
            BackCommand.DisplayName = FText::FromString(TEXT("Back"));
            BackCommand.CommandType = ERTSCommandType::Back;
            BackCommand.Hotkey = EKeys::Escape;
            BackCommand.CommandPage = ActiveCommandPage;
            BackCommand.SortOrder = -1000;
            OutCommands.Add(BackCommand);
        }

        AppendWorkerBuildCommands(OutCommands, SlotIndex);
        return;
    }

    AppendStopCommand(OutCommands, SlotIndex);


    AppendProductionCommands(OutCommands, SlotIndex);


    AppendWorkerBuildCommands(OutCommands, SlotIndex);
}

void ARTSPlayerController::AppendStopCommand(TArray<FRTSCommandButton>& OutCommands, int32& SlotIndex) const
{
    if (!ActiveCommandPage.IsNone() || !HasStoppableSelection())
    {
        return;
    }

    if (MaxCommandCardSlots > 0 && OutCommands.Num() >= MaxCommandCardSlots)
    {
        return;
    }

    FRTSCommandButton Command;
    Command.SlotIndex = SlotIndex++;
    Command.CommandId = TEXT("Stop");
    Command.DisplayName = FText::FromString(TEXT("Stop"));
    Command.TooltipText = FText::FromString(TEXT("Stop current movement, attack, gather, build, or flying movement commands."));
    Command.CommandType = ERTSCommandType::Stop;
    Command.Hotkey = StopCommandHotkey;
    Command.Icon = StopCommandIcon;
    Command.SortOrder = -500;
    Command.bEnabled = true;

    OutCommands.Add(Command);
}
void ARTSPlayerController::AppendProductionCommands(TArray<FRTSCommandButton>& OutCommands, int32& SlotIndex) const
{
    if (!ActiveCommandPage.IsNone())
    {
        return;
    }

    TArray<URTSUnitData*> AvailableUnits;
    TSet<URTSUnitData*> AddedUnits;

    for (ARTSBuilding* Building : GetOwnedSelectedBuildings())
    {
        if (!Building || !Building->BuildingData || Building->BuildingState != ERTSBuildingState::Completed)
        {
            continue;
        }

        for (TObjectPtr<URTSUnitData> TrainableUnitPtr : Building->BuildingData->TrainableUnits)
        {
            URTSUnitData* UnitData = TrainableUnitPtr.Get();
            if (!UnitData || AddedUnits.Contains(UnitData))
            {
                continue;
            }

            AvailableUnits.Add(UnitData);
            AddedUnits.Add(UnitData);
        }
    }

    AvailableUnits.Sort([](const URTSUnitData& Left, const URTSUnitData& Right)
    {
        if (Left.CommandCardOrder != Right.CommandCardOrder)
        {
            return Left.CommandCardOrder < Right.CommandCardOrder;
        }

        return Left.UnitId.ToString() < Right.UnitId.ToString();
    });

    for (URTSUnitData* UnitData : AvailableUnits)
    {
        if (MaxCommandCardSlots > 0 && OutCommands.Num() >= MaxCommandCardSlots)
        {
            return;
        }

        FRTSCommandButton Command;
        Command.SlotIndex = SlotIndex++;
        Command.CommandId = UnitData->UnitId;
        Command.DisplayName = UnitData->DisplayName.IsEmpty()
            ? FText::FromName(UnitData->UnitId)
            : UnitData->DisplayName;
        Command.CommandType = ERTSCommandType::TrainUnit;
        Command.Hotkey = UnitData->CommandHotkey;
        Command.CommandPage = NAME_None;
        Command.UnitData = UnitData;
        PopulateUnitCommandMetadata(Command, UnitData);

        OutCommands.Add(Command);
    }
}

void ARTSPlayerController::AppendWorkerBuildCommands(TArray<FRTSCommandButton>& OutCommands, int32& SlotIndex) const
{
    TArray<ARTSWorkerUnit*> Workers = GetOwnedSelectedWorkers();
    if (Workers.Num() == 0)
    {
        return;
    }

    if (ActiveCommandPage.IsNone())
    {
        TSet<FName> AddedPages;

        for (ARTSWorkerUnit* Worker : Workers)
        {
            if (!Worker || !Worker->UnitData || !Worker->UnitData->bWorker || Worker->UnitData->BuildableBuildings.Num() == 0)
            {
                continue;
            }

            const FName PageName = Worker->UnitData->BuildCommandPage.IsNone()
                ? FName(TEXT("Build"))
                : Worker->UnitData->BuildCommandPage;

            if (AddedPages.Contains(PageName))
            {
                continue;
            }

            if (MaxCommandCardSlots > 0 && OutCommands.Num() >= MaxCommandCardSlots)
            {
                return;
            }

            FRTSCommandButton Command;
            Command.SlotIndex = SlotIndex++;
            Command.CommandId = PageName;
            Command.DisplayName = Worker->UnitData->BuildMenuDisplayName.IsEmpty()
                ? FText::FromString(TEXT("Build"))
                : Worker->UnitData->BuildMenuDisplayName;
            Command.CommandType = ERTSCommandType::OpenCommandPage;
            Command.Hotkey = Worker->UnitData->BuildMenuHotkey.IsValid()
                ? Worker->UnitData->BuildMenuHotkey
                : EKeys::B;
            Command.CommandPage = NAME_None;
            Command.TargetCommandPage = PageName;
            Command.Icon = Worker->UnitData->BuildMenuIcon;
            Command.SortOrder = -100;
            Command.bEnabled = true;

            OutCommands.Add(Command);
            AddedPages.Add(PageName);
        }

        return;
    }

    TArray<URTSBuildingData*> AvailableBuildings;
    TSet<URTSBuildingData*> AddedBuildings;

    for (ARTSWorkerUnit* Worker : Workers)
    {
        if (!Worker || !Worker->UnitData || !Worker->UnitData->bWorker)
        {
            continue;
        }

        const FName DefaultWorkerPage = Worker->UnitData->BuildCommandPage.IsNone()
            ? FName(TEXT("Build"))
            : Worker->UnitData->BuildCommandPage;

        for (TObjectPtr<URTSBuildingData> BuildableBuildingPtr : Worker->UnitData->BuildableBuildings)
        {
            URTSBuildingData* BuildingData = BuildableBuildingPtr.Get();
            if (!BuildingData || AddedBuildings.Contains(BuildingData))
            {
                continue;
            }

            const FName BuildingPage = BuildingData->WorkerCommandPage.IsNone()
                ? DefaultWorkerPage
                : BuildingData->WorkerCommandPage;

            if (BuildingPage != ActiveCommandPage)
            {
                continue;
            }

            AvailableBuildings.Add(BuildingData);
            AddedBuildings.Add(BuildingData);
        }
    }

    AvailableBuildings.Sort([](const URTSBuildingData& Left, const URTSBuildingData& Right)
    {
        if (Left.CommandCardOrder != Right.CommandCardOrder)
        {
            return Left.CommandCardOrder < Right.CommandCardOrder;
        }

        return Left.BuildingId.ToString() < Right.BuildingId.ToString();
    });

    for (URTSBuildingData* BuildingData : AvailableBuildings)
    {
        if (MaxCommandCardSlots > 0 && OutCommands.Num() >= MaxCommandCardSlots)
        {
            return;
        }

        FRTSCommandButton Command;
        Command.SlotIndex = SlotIndex++;
        Command.CommandId = BuildingData->BuildingId;
        Command.DisplayName = BuildingData->DisplayName.IsEmpty()
            ? FText::FromName(BuildingData->BuildingId)
            : BuildingData->DisplayName;
        Command.CommandType = ERTSCommandType::BuildStructure;
        Command.Hotkey = BuildingData->CommandHotkey;
        Command.CommandPage = ActiveCommandPage;
        Command.BuildingData = BuildingData;
        PopulateBuildingCommandMetadata(Command, BuildingData);

        OutCommands.Add(Command);
    }
}

bool ARTSPlayerController::ExecuteCommandButton(const FRTSCommandButton& Command)
{
    if (!Command.bEnabled)
    {
        return false;
    }

    switch (Command.CommandType)
    {
    case ERTSCommandType::TrainUnit:
        return QueueProductionForSelectedBuilding(Command.UnitData.Get());

    case ERTSCommandType::BuildStructure:
        if (!Command.BuildingData || GetOwnedSelectedWorkers().Num() == 0)
        {
            return false;
        }

        StartBuildMode(Command.BuildingData.Get());
        ClearCommandPage();
        return true;

    case ERTSCommandType::OpenCommandPage:
        SetActiveCommandPage(Command.TargetCommandPage);
        return true;

    case ERTSCommandType::Stop:
        return StopSelectedCommands();

    case ERTSCommandType::Back:
        ClearCommandPage();
        return true;

    default:
        return false;
    }
}

bool ARTSPlayerController::StopSelectedCommands()
{
    const TArray<ARTSUnitBase*> Units = GetOwnedSelectedUnits();
    const TArray<ARTSBuilding*> Buildings = GetOwnedSelectedBuildings();

    if (Units.Num() == 0 && Buildings.Num() == 0)
    {
        return false;
    }

    if (!HasStoppableSelection())
    {
        return false;
    }

    if (HasAuthority())
    {
        StopCommandsOnServer(Units, Buildings);
    }
    else
    {
        ServerStopCommands(Units, Buildings);
    }

    return true;
}
bool ARTSPlayerController::ExecuteCommandSlot(int32 SlotIndex)
{
    TArray<FRTSCommandButton> Commands;
    GetAvailableCommandButtons(Commands);

    for (const FRTSCommandButton& Command : Commands)
    {
        if (Command.SlotIndex == SlotIndex)
        {
            return ExecuteCommandButton(Command);
        }
    }

    return false;
}

bool ARTSPlayerController::ExecuteCommandHotkey(FKey Hotkey)
{
    if (!Hotkey.IsValid())
    {
        return false;
    }

    TArray<FRTSCommandButton> Commands;
    GetAvailableCommandButtons(Commands);

    for (const FRTSCommandButton& Command : Commands)
    {
        if (Command.Hotkey == Hotkey)
        {
            return ExecuteCommandButton(Command);
        }
    }

    return false;
}

void ARTSPlayerController::SetActiveCommandPage(FName CommandPage)
{
    ActiveCommandPage = CommandPage;
    OnSelectionChanged.Broadcast();
}

void ARTSPlayerController::ClearCommandPage()
{
    if (ActiveCommandPage.IsNone())
    {
        return;
    }

    ActiveCommandPage = NAME_None;
    OnSelectionChanged.Broadcast();
}

bool ARTSPlayerController::CanBuildingTrainUnit(const ARTSBuilding* Building, const URTSUnitData* UnitData) const
{
    if (!Building || !Building->BuildingData || !UnitData)
    {
        return false;
    }

    for (TObjectPtr<URTSUnitData> TrainableUnitPtr : Building->BuildingData->TrainableUnits)
    {
        if (TrainableUnitPtr.Get() == UnitData)
        {
            return true;
        }
    }

    return false;
}

void ARTSPlayerController::HandleControlGroupInput(int32 GroupIndex)
{
    const bool bCtrlDown = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
    const bool bShiftDown = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

    if (bCtrlDown && bShiftDown)
    {
        AddSelectionToControlGroup(GroupIndex);
        return;
    }

    if (bCtrlDown)
    {
        AssignControlGroup(GroupIndex);
        return;
    }

    RecallControlGroup(GroupIndex, bShiftDown);
}

void ARTSPlayerController::AssignControlGroup(int32 GroupIndex)
{
    if (!IsValidControlGroupIndex(GroupIndex))
    {
        return;
    }

    FRTSControlGroup& Group = ControlGroups.FindOrAdd(GroupIndex);
    Group.Actors.Reset();
    Group.LastRecallTimeSeconds = -1.0;

    for (AActor* SelectedActor : SelectedActors)
    {
        if (IsActorSelectable(SelectedActor) && IsOwnedByLocalPlayer(SelectedActor))
        {
            Group.Actors.AddUnique(SelectedActor);
        }
    }
}

void ARTSPlayerController::AddSelectionToControlGroup(int32 GroupIndex)
{
    if (!IsValidControlGroupIndex(GroupIndex))
    {
        return;
    }

    FRTSControlGroup& Group = ControlGroups.FindOrAdd(GroupIndex);
    Group.LastRecallTimeSeconds = -1.0;

    for (AActor* SelectedActor : SelectedActors)
    {
        if (IsActorSelectable(SelectedActor) && IsOwnedByLocalPlayer(SelectedActor))
        {
            Group.Actors.AddUnique(SelectedActor);
        }
    }
}

bool ARTSPlayerController::RecallControlGroup(int32 GroupIndex, bool bAppendSelection)
{
    FRTSControlGroup* Group = ControlGroups.Find(GroupIndex);
    if (!Group)
    {
        return false;
    }

    PruneControlGroup(*Group);
    if (Group->Actors.Num() == 0)
    {
        return false;
    }

    UWorld* World = GetWorld();
    const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
    const bool bDoubleTap = Group->LastRecallTimeSeconds >= 0.0
        && CurrentTimeSeconds - Group->LastRecallTimeSeconds <= ControlGroupDoubleTapSeconds;

    Group->LastRecallTimeSeconds = CurrentTimeSeconds;

    if (!bAppendSelection)
    {
        ClearSelection();
    }

    for (AActor* ActorInGroup : Group->Actors)
    {
        SelectActor(ActorInGroup, true);
    }

    if (bDoubleTap)
    {
        MoveCameraToActorGroup(Group->Actors);
    }

    return true;
}

void ARTSPlayerController::ClearControlGroup(int32 GroupIndex)
{
    ControlGroups.Remove(GroupIndex);
}

bool ARTSPlayerController::IsValidControlGroupIndex(int32 GroupIndex) const
{
    return GroupIndex >= 0 && GroupIndex <= 9;
}
bool ARTSPlayerController::HasStoppableSelection() const
{
    if (GetOwnedSelectedUnits().Num() > 0)
    {
        return true;
    }

    return GetOwnedSelectedFlyingTerranBuildings().Num() > 0;
}

void ARTSPlayerController::PruneControlGroup(FRTSControlGroup& Group) const
{
    for (int32 Index = Group.Actors.Num() - 1; Index >= 0; --Index)
    {
        AActor* Actor = Group.Actors[Index].Get();
        if (!IsActorSelectable(Actor) || !IsOwnedByLocalPlayer(Actor))
        {
            Group.Actors.RemoveAt(Index);
        }
    }
}

bool ARTSPlayerController::GetActorGroupCenter(const TArray<TObjectPtr<AActor>>& Actors, FVector& OutCenter) const
{
    FVector AccumulatedLocation = FVector::ZeroVector;
    int32 ValidActorCount = 0;

    for (const TObjectPtr<AActor>& ActorPtr : Actors)
    {
        AActor* Actor = ActorPtr.Get();
        if (!Actor)
        {
            continue;
        }

        AccumulatedLocation += Actor->GetActorLocation();
        ++ValidActorCount;
    }

    if (ValidActorCount == 0)
    {
        return false;
    }

    OutCenter = AccumulatedLocation / static_cast<float>(ValidActorCount);
    return true;
}

void ARTSPlayerController::MoveCameraToActorGroup(const TArray<TObjectPtr<AActor>>& Actors)
{
    FVector GroupCenter;
    if (!GetActorGroupCenter(Actors, GroupCenter))
    {
        return;
    }

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        return;
    }

    FVector CameraPawnLocation = ControlledPawn->GetActorLocation();
    CameraPawnLocation.X = GroupCenter.X;
    CameraPawnLocation.Y = GroupCenter.Y;
    ControlledPawn->SetActorLocation(CameraPawnLocation, false);
}

bool ARTSPlayerController::IsAttackCommandModifierDown() const
{
    return bUseAttackCommandModifier
        && AttackCommandModifierKey.IsValid()
        && IsInputKeyDown(AttackCommandModifierKey);
}

bool ARTSPlayerController::IsEnemyActorForLocalPlayer(AActor* Actor) const
{
    if (!Actor || !Actor->FindComponentByClass<URTSHealthComponent>())
    {
        return false;
    }

    const ARTSPlayerState* LocalPlayerState = GetPlayerState<ARTSPlayerState>();
    if (!LocalPlayerState || !Actor->GetClass()->ImplementsInterface(URTSSelectableInterface::StaticClass()))
    {
        return false;
    }

    const int32 TargetTeam = IRTSSelectableInterface::Execute_GetSelectableTeamNumber(Actor);
    return TargetTeam >= 0
        && TargetTeam != LocalPlayerState->TeamNumber
        && IsActorVisibleToLocalPlayer(Actor);
}

void ARTSPlayerController::ShowCommandFeedback(const FVector& WorldLocation, bool bTargetActorFeedback, bool bAttackCommandFeedback) const
{
    if (!bShowCommandFeedback || !GetWorld())
    {
        return;
    }

    UMaterialInterface* DecalMaterial = nullptr;
    if (bAttackCommandFeedback)
    {
        DecalMaterial = AttackCommandDecalMaterial;
    }
    else
    {
        DecalMaterial = bTargetActorFeedback ? TargetCommandDecalMaterial : GroundCommandDecalMaterial;
    }

    const FVector MarkerLocation = WorldLocation + FVector(0.0f, 0.0f, 8.0f);
    if (DecalMaterial)
    {
        UGameplayStatics::SpawnDecalAtLocation(
            GetWorld(),
            DecalMaterial,
            CommandFeedbackDecalSize,
            MarkerLocation,
            FRotator(-90.0f, 0.0f, 0.0f),
            CommandFeedbackDuration
        );
        return;
    }

    const FColor MarkerColor = bAttackCommandFeedback
        ? FColor::Red
        : (bTargetActorFeedback ? FColor::Yellow : FColor::Green);
    const float Radius = bTargetActorFeedback ? CommandFeedbackRadius * 0.75f : CommandFeedbackRadius;

    DrawDebugCircle(
        GetWorld(),
        MarkerLocation,
        Radius,
        32,
        MarkerColor,
        false,
        CommandFeedbackDuration,
        0,
        3.0f,
        FVector::ForwardVector,
        FVector::RightVector,
        false
    );

    if (bTargetActorFeedback || bAttackCommandFeedback)
    {
        DrawDebugSphere(
            GetWorld(),
            MarkerLocation + FVector(0.0f, 0.0f, 35.0f),
            22.0f,
            12,
            MarkerColor,
            false,
            CommandFeedbackDuration,
            0,
            2.0f
        );
    }
}

void ARTSPlayerController::IssueSmartCommand()
{
    if (bIsInBuildMode)
    {
        return;
    }

    FVector2D MousePosition;
    if (!GetClampedMousePosition(MousePosition))
    {
        return;
    }

    FHitResult HitResult;
    const bool bHit = GetHitResultAtScreenPosition(
        MousePosition,
        SelectionTraceChannel,
        false,
        HitResult
    );

    if (!bHit)
    {
        return;
    }

    AActor* HitActor = HitResult.GetActor();
    const bool bAttackCommand = IsAttackCommandModifierDown();
    const bool bEnemyTarget = IsEnemyActorForLocalPlayer(HitActor);
    const bool bTargetActorFeedback = bEnemyTarget || Cast<ARTSResourceNode>(HitActor) || Cast<ARTSBuilding>(HitActor);

    ShowCommandFeedback(
        bTargetActorFeedback && HitActor ? HitActor->GetActorLocation() : HitResult.Location,
        bTargetActorFeedback,
        bAttackCommand
    );

    const TArray<ARTSUnitBase*> Units = GetOwnedSelectedUnits();
    if (bAttackCommand && Units.Num() > 0)
    {
        if (bEnemyTarget)
        {
            if (HasAuthority())
            {
                IssueAttackTargetCommandOnServer(Units, HitActor);
            }
            else
            {
                ServerIssueAttackTargetCommand(Units, HitActor);
            }
            return;
        }

        if (HasAuthority())
        {
            IssueAttackMoveCommandOnServer(Units, HitResult.Location);
        }
        else
        {
            ServerIssueAttackMoveCommand(Units, HitResult.Location);
        }
        return;
    }

    const TArray<ARTSBuilding*> Buildings = GetOwnedSelectedBuildings();

    if (ARTSResourceNode* ResourceNode = Cast<ARTSResourceNode>(HitActor))
    {
        const TArray<ARTSWorkerUnit*> Workers = GetOwnedSelectedWorkers();
        if (Workers.Num() > 0)
        {
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

        if (Buildings.Num() > 0)
        {
            if (HasAuthority())
            {
                IssueRallyPointCommandOnServer(Buildings, ResourceNode->GetGatherLocation(), ResourceNode);
            }
            else
            {
                ServerSetRallyPoint(Buildings, ResourceNode->GetGatherLocation(), ResourceNode);
            }
        }
        return;
    }

    if (Units.Num() > 0)
    {
        if (HasAuthority())
        {
            IssueMoveCommandOnServer(Units, HitResult.Location);
        }
        else
        {
            ServerIssueMoveCommand(Units, HitResult.Location);
        }
        return;
    }

    if (Buildings.Num() == 0)
    {
        return;
    }

    const TArray<ARTSTerranBuilding*> FlyingBuildings = GetOwnedSelectedFlyingTerranBuildings();
    if (FlyingBuildings.Num() > 0)
    {
        if (HasAuthority())
        {
            IssueFlyingBuildingMoveCommandOnServer(FlyingBuildings, HitResult.Location);
        }
        else
        {
            ServerIssueFlyingBuildingMoveCommand(FlyingBuildings, HitResult.Location);
        }
        return;
    }

    if (HasAuthority())
    {
        IssueRallyPointCommandOnServer(Buildings, HitResult.Location, nullptr);
    }
    else
    {
        ServerSetRallyPoint(Buildings, HitResult.Location, nullptr);
    }
}
void ARTSPlayerController::ServerIssueMoveCommand_Implementation(
    const TArray<ARTSUnitBase*>& Units,
    FVector TargetLocation
)
{
    IssueMoveCommandOnServer(Units, TargetLocation);
}

void ARTSPlayerController::ServerIssueAttackTargetCommand_Implementation(
    const TArray<ARTSUnitBase*>& Units,
    AActor* TargetActor
)
{
    IssueAttackTargetCommandOnServer(Units, TargetActor);
}

void ARTSPlayerController::ServerIssueAttackMoveCommand_Implementation(
    const TArray<ARTSUnitBase*>& Units,
    FVector TargetLocation
)
{
    IssueAttackMoveCommandOnServer(Units, TargetLocation);
}
void ARTSPlayerController::ServerStopCommands_Implementation(
    const TArray<ARTSUnitBase*>& Units,
    const TArray<ARTSBuilding*>& Buildings
)
{
    StopCommandsOnServer(Units, Buildings);
}

void ARTSPlayerController::ServerIssueFlyingBuildingMoveCommand_Implementation(
    const TArray<ARTSTerranBuilding*>& Buildings,
    FVector TargetLocation
)
{
    IssueFlyingBuildingMoveCommandOnServer(Buildings, TargetLocation);
}
void ARTSPlayerController::ServerIssueGatherCommand_Implementation(
    const TArray<ARTSWorkerUnit*>& Workers,
    ARTSResourceNode* ResourceNode
)
{
    IssueGatherCommandOnServer(Workers, ResourceNode);
}


void ARTSPlayerController::ServerSetRallyPoint_Implementation(
    const TArray<ARTSBuilding*>& Buildings,
    FVector TargetLocation,
    ARTSResourceNode* ResourceTarget
)
{
    IssueRallyPointCommandOnServer(Buildings, TargetLocation, ResourceTarget);
}
void ARTSPlayerController::IssueAttackTargetCommandOnServer(
    const TArray<ARTSUnitBase*>& Units,
    AActor* TargetActor
)
{
    if (!HasAuthority() || !TargetActor)
    {
        return;
    }

    for (ARTSUnitBase* Unit : Units)
    {
        if (!Unit || !Unit->CanReceiveCommandsFrom(this) || !Unit->AttackComponent)
        {
            continue;
        }
        Unit->StopAllCommands();
        Unit->AttackComponent->IssueAttackTarget(TargetActor);
    }
}

void ARTSPlayerController::IssueAttackMoveCommandOnServer(
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
        Unit->StopAllCommands();

        const int32 Row = Index / 3;
        const int32 Col = Index % 3;
        const FVector FormationOffset((Col - 1) * 85.0f, Row * 85.0f, 0.0f);
        const FVector UnitAttackMoveLocation = TargetLocation + FormationOffset;

        if (!Unit->AttackComponent || !Unit->AttackComponent->IssueAttackMove(UnitAttackMoveLocation))
        {
            Unit->IssueMoveCommand(UnitAttackMoveLocation);
        }
    }
}
void ARTSPlayerController::StopCommandsOnServer(
    const TArray<ARTSUnitBase*>& Units,
    const TArray<ARTSBuilding*>& Buildings
)
{
    if (!HasAuthority())
    {
        return;
    }

    for (ARTSUnitBase* Unit : Units)
    {
        if (!Unit || !Unit->CanReceiveCommandsFrom(this))
        {
            continue;
        }

        Unit->StopAllCommands();
    }

    for (ARTSBuilding* Building : Buildings)
    {
        if (!Building || !Building->CanReceiveCommandsFrom(this))
        {
            continue;
        }

        Building->StopAllCommands();
    }
}

void ARTSPlayerController::IssueFlyingBuildingMoveCommandOnServer(
    const TArray<ARTSTerranBuilding*>& Buildings,
    const FVector& TargetLocation
)
{
    if (!HasAuthority())
    {
        return;
    }

    for (int32 Index = 0; Index < Buildings.Num(); ++Index)
    {
        ARTSTerranBuilding* Building = Buildings[Index];
        if (!Building || !Building->CanReceiveCommandsFrom(this) || Building->BuildingState != ERTSBuildingState::Flying)
        {
            continue;
        }

        const int32 Row = Index / 3;
        const int32 Col = Index % 3;
        const FVector FormationOffset((Col - 1) * 120.0f, Row * 120.0f, 0.0f);
        Building->IssueFlyingMoveCommand(TargetLocation + FormationOffset);
    }
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
        Unit->StopAllCommands();

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

void ARTSPlayerController::IssueRallyPointCommandOnServer(
    const TArray<ARTSBuilding*>& Buildings,
    const FVector& TargetLocation,
    ARTSResourceNode* ResourceTarget
)
{
    if (!HasAuthority())
    {
        return;
    }

    for (ARTSBuilding* Building : Buildings)
    {
        if (!Building || !Building->CanReceiveCommandsFrom(this))
        {
            continue;
        }

        Building->SetProductionRallyPointTarget(TargetLocation, ResourceTarget);
    }
}
bool ARTSPlayerController::QueueProductionForSelectedBuilding(URTSUnitData* UnitData)
{
    if (!UnitData)
    {
        return false;
    }

    for (ARTSBuilding* Building : GetOwnedSelectedBuildings())
    {
        if (!CanBuildingTrainUnit(Building, UnitData))
        {
            continue;
        }

        if (HasAuthority())
        {
            if (QueueProductionOnServer(Building, UnitData->UnitId))
            {
                return true;
            }
            continue;
        }

        ServerQueueProduction(Building, UnitData->UnitId);
        return true;
    }

    return false;
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
    if (!UnitData && Building && Building->BuildingData)
    {
        // Fallback to the selected producer data so UI-driven commands do not require duplicate controller lists.
        for (TObjectPtr<URTSUnitData> TrainableUnitPtr : Building->BuildingData->TrainableUnits)
        {
            URTSUnitData* TrainableUnit = TrainableUnitPtr.Get();
            if (TrainableUnit && TrainableUnit->UnitId == UnitId)
            {
                UnitData = TrainableUnit;
                break;
            }
        }
    }

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

bool ARTSPlayerController::StartWorkerBuildOrderOnServer(
    ARTSWorkerUnit* Worker,
    FName BuildingId,
    FRTSGridCoord OriginCoord
)
{
    if (!HasAuthority() || !Worker || !Worker->CanReceiveCommandsFrom(this))
    {
        return false;
    }

    if (!GridManager)
    {
        GridManager = ResolveGridManager();
    }

    if (!GridManager)
    {
        return false;
    }

    URTSBuildingData* BuildingData = FindBuildableBuildingDataForWorker(Worker, BuildingId);
    if (!BuildingData || !GridManager->CanPlaceBuildingByData(OriginCoord, BuildingData))
    {
        return false;
    }

    ARTSPlayerState* RTSPlayerState = GetPlayerState<ARTSPlayerState>();
    if (!RTSPlayerState || !RTSPlayerState->CanAfford(BuildingData->MineralCost, BuildingData->VespeneCost))
    {
        return false;
    }

    if (!Worker->BuildComponent)
    {
        return false;
    }

    return Worker->BuildComponent->StartBuildOrder(this, BuildingData, OriginCoord, GridManager);
}

bool ARTSPlayerController::CompleteWorkerBuildOrder(
    ARTSWorkerUnit* Worker,
    FName BuildingId,
    FRTSGridCoord OriginCoord
)
{
    if (!HasAuthority() || !Worker || !Worker->CanReceiveCommandsFrom(this))
    {
        return false;
    }

    URTSBuildingData* BuildingData = FindBuildableBuildingDataForWorker(Worker, BuildingId);
    if (!BuildingData || !IsWorkerCloseEnoughToBuild(Worker, BuildingData, OriginCoord))
    {
        return false;
    }

    if (!GridManager)
    {
        GridManager = ResolveGridManager();
    }

    ARTSPlayerState* RTSPlayerState = GetPlayerState<ARTSPlayerState>();
    if (!GridManager
        || !GridManager->CanPlaceBuildingByData(OriginCoord, BuildingData)
        || !RTSPlayerState
        || !RTSPlayerState->CanAfford(BuildingData->MineralCost, BuildingData->VespeneCost))
    {
        return false;
    }

    BuildOnServer(BuildingId, OriginCoord);
    return true;
}

bool ARTSPlayerController::IsWorkerCloseEnoughToBuild(
    ARTSWorkerUnit* Worker,
    URTSBuildingData* BuildingData,
    FRTSGridCoord OriginCoord
)
{
    if (!Worker || !BuildingData)
    {
        return false;
    }

    if (!GridManager)
    {
        GridManager = ResolveGridManager();
    }

    if (!GridManager)
    {
        return false;
    }

    FVector BuildLocation;
    if (!GridManager->GetBuildingCenterLocationOnGround(
        OriginCoord,
        FMath::Max(1, BuildingData->GridWidth),
        FMath::Max(1, BuildingData->GridHeight),
        BuildLocation
    ))
    {
        return false;
    }

    return FVector::DistSquared(Worker->GetActorLocation(), BuildLocation)
        <= FMath::Square(WorkerBuildStartAcceptanceRadius);
}

URTSBuildingData* ARTSPlayerController::FindBuildableBuildingDataForWorker(
    ARTSWorkerUnit* Worker,
    FName BuildingId
) const
{
    if (!Worker || !Worker->UnitData)
    {
        return nullptr;
    }

    for (TObjectPtr<URTSBuildingData> BuildableBuildingPtr : Worker->UnitData->BuildableBuildings)
    {
        URTSBuildingData* BuildingData = BuildableBuildingPtr.Get();
        if (BuildingData && BuildingData->BuildingId == BuildingId)
        {
            return BuildingData;
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


    for (URTSUnitData* UnitData : UnitDataList)
    {
        if (!UnitData)
        {
            continue;
        }

        for (TObjectPtr<URTSBuildingData> BuildableBuildingPtr : UnitData->BuildableBuildings)
        {
            URTSBuildingData* BuildableBuilding = BuildableBuildingPtr.Get();
            if (BuildableBuilding && BuildableBuilding->BuildingId == BuildingId)
            {
                return BuildableBuilding;
            }
        }
    }

    // Search worker command data as a fallback for server-side build completion.
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<ARTSWorkerUnit> It(World); It; ++It)
        {
            ARTSWorkerUnit* Worker = *It;
            if (!Worker || !Worker->UnitData)
            {
                continue;
            }

            for (TObjectPtr<URTSBuildingData> BuildableBuildingPtr : Worker->UnitData->BuildableBuildings)
            {
                URTSBuildingData* BuildableBuilding = BuildableBuildingPtr.Get();
                if (BuildableBuilding && BuildableBuilding->BuildingId == BuildingId)
                {
                    return BuildableBuilding;
                }
            }
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
