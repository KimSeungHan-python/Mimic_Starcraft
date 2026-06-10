#include "Buildings/RTSBuilding.h"
#include "Components/RTSProductionQueueComponent.h"
#include "Components/DecalComponent.h"
#include "Components/RTSCombatEffectsComponent.h"
#include "Components/RTSHealthComponent.h"
#include "Core/RTSPlayerState.h"
#include "Data/RTSBuildingData.h"
#include "Data/RTSUnitData.h"
#include "Grid/RTSGridManager.h"

#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Spatial/RTSActorSpatialIndex.h"

ARTSBuilding::ARTSBuilding()
{
    bReplicates = true;
    SetReplicateMovement(true);

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(SceneRoot);

    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
    MeshComponent->SetCanEverAffectNavigation(true);

    SelectionDecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("SelectionDecalComponent"));
    SelectionDecalComponent->SetupAttachment(SceneRoot);
    SelectionDecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    SelectionDecalComponent->SetRelativeLocation(FVector(0.0f, 0.0f, SelectionDecalZOffset));
    SelectionDecalComponent->DecalSize = FVector(32.0f, 240.0f, 240.0f);
    SelectionDecalComponent->SetHiddenInGame(true);
    SelectionDecalComponent->SetVisibility(false);
    SelectionDecalComponent->SetFadeScreenSize(0.0f);

    ProductionQueueComponent = CreateDefaultSubobject<URTSProductionQueueComponent>(TEXT("ProductionQueueComponent"));
    HealthComponent = CreateDefaultSubobject<URTSHealthComponent>(TEXT("HealthComponent"));
    CombatEffectsComponent = CreateDefaultSubobject<URTSCombatEffectsComponent>(TEXT("CombatEffectsComponent"));
}

void ARTSBuilding::BeginPlay()
{
    Super::BeginPlay();

    if (!OwningGridManager)
    {
        OwningGridManager = ResolveGridManager();
    }

    RefreshBuildingVisual();

    if (HasAuthority() && !bIsPreviewBuilding)
    {
        if (ARTSActorSpatialIndex* SpatialIndex = ARTSActorSpatialIndex::GetOrCreate(GetWorld()))
        {
            SpatialIndex->RegisterActor(this);
        }
    }

    if (!HasAuthority())
    {
        RegisterToLocalGridIfNeeded();
    }

    SetActorTickEnabled(HasAuthority() && BuildingState == ERTSBuildingState::UnderConstruction);
}

void ARTSBuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority())
    {
        if (ARTSActorSpatialIndex* SpatialIndex = ARTSActorSpatialIndex::FindExisting(GetWorld()))
        {
            SpatialIndex->UnregisterActor(this);
        }
    }

    UnregisterFromGrid();
    Super::EndPlay(EndPlayReason);
}

void ARTSBuilding::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!HasAuthority())
    {
        return;
    }

    if (BuildingState != ERTSBuildingState::UnderConstruction)
    {
        return;
    }

    CurrentBuildTime += DeltaTime;

    if (CurrentBuildTime >= BuildTime)
    {
        CompleteConstruction();
    }

}

void ARTSBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARTSBuilding, BuildingData);
    DOREPLIFETIME(ARTSBuilding, GridOriginCoord);
    DOREPLIFETIME(ARTSBuilding, GridWidth);
    DOREPLIFETIME(ARTSBuilding, GridHeight);
    DOREPLIFETIME(ARTSBuilding, CachedCellSize);
    DOREPLIFETIME(ARTSBuilding, BuildingState);
    DOREPLIFETIME(ARTSBuilding, BuildTime);
    DOREPLIFETIME(ARTSBuilding, BuildStartServerTime);
    DOREPLIFETIME(ARTSBuilding, TeamNumber);
    DOREPLIFETIME(ARTSBuilding, TeamColor);
    DOREPLIFETIME(ARTSBuilding, OwningPlayerState);
}

void ARTSBuilding::InitializeBuilding(
    URTSBuildingData* InBuildingData,
    FRTSGridCoord InGridOriginCoord,
    int32 InGridWidth,
    int32 InGridHeight,
    float InCellSize,
    ARTSGridManager* InGridManager
)
{
    BuildingData = InBuildingData;
    GridOriginCoord = InGridOriginCoord;
    GridWidth = InGridWidth;
    GridHeight = InGridHeight;
    CachedCellSize = InCellSize;
    OwningGridManager = InGridManager;

    RefreshBuildingVisual();

    if (!HasAuthority())
    {
        RegisterToLocalGridIfNeeded();
    }

    SetActorTickEnabled(HasAuthority() && BuildingState == ERTSBuildingState::UnderConstruction);
}

void ARTSBuilding::FitMeshToGridFootprint(float CellSize)
{
    if (!MeshComponent)
    {
        return;
    }

    UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();

    if (!StaticMesh)
    {
        return;
    }

    const FBoxSphereBounds MeshBounds = StaticMesh->GetBounds();

    const FVector MeshSize = MeshBounds.BoxExtent * 2.0f;

    if (MeshSize.X <= KINDA_SMALL_NUMBER || MeshSize.Y <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const float TargetWorldSizeX = GridWidth * CellSize;
    const float TargetWorldSizeY = GridHeight * CellSize;

    FVector NewScale = MeshComponent->GetRelativeScale3D();

    NewScale.X = TargetWorldSizeX / MeshSize.X;
    NewScale.Y = TargetWorldSizeY / MeshSize.Y;

    // Z는 건물 높이이므로 일단 유지.
    // 필요하면 나중에 따로 DesiredWorldHeight 같은 값을 만들어서 조절하는 게 좋습니다.
    MeshComponent->SetRelativeScale3D(NewScale);

    /*
     * Mesh 피벗이 중앙이 아니어도,
     * Actor 위치를 기준으로 Mesh의 X/Y 중앙이 맞도록 보정합니다.
     * Z는 Mesh 바닥이 Actor 위치의 Z에 닿도록 보정합니다.
     */
    FVector NewRelativeLocation = FVector::ZeroVector;

    NewRelativeLocation.X = -MeshBounds.Origin.X * NewScale.X;
    NewRelativeLocation.Y = -MeshBounds.Origin.Y * NewScale.Y;
    NewRelativeLocation.Z = (MeshBounds.BoxExtent.Z - MeshBounds.Origin.Z) * NewScale.Z;



    MeshComponent->SetRelativeLocation(NewRelativeLocation);

    //UE_LOG(LogTemp, Warning, TEXT("MeshBounds Origin: %s"),
    //    *MeshBounds.Origin.ToString());

    //UE_LOG(LogTemp, Warning, TEXT("MeshBounds Extent: %s"),
    //    *MeshBounds.BoxExtent.ToString());

    //UE_LOG(LogTemp, Warning, TEXT("Mesh NewScale: %s"),
    //    *NewScale.ToString());

    //UE_LOG(LogTemp, Warning, TEXT("Mesh NewRelativeLocation: %s"),
    //    *NewRelativeLocation.ToString());
}

void ARTSBuilding::BeginConstruction(float InBuildTime)
{
    if (!HasAuthority())
    {
        return;
    }


    BuildTime = FMath::Max(0.01f, InBuildTime);
    CurrentBuildTime = 0.0f;
    BuildStartServerTime = GetSyncedServerTimeSeconds();
    BuildingState = ERTSBuildingState::UnderConstruction;
    RegisterToGrid();
    bIsCompleted = false;
    bCompletedGridEffectsApplied = false;
    SetActorTickEnabled(true);

    OnRep_BuildingState();
}

void ARTSBuilding::CompleteConstruction()
{
    if (!HasAuthority())
    {
        return;
    }

    CurrentBuildTime = BuildTime;
    bIsCompleted = true;

    BuildingState = ERTSBuildingState::Completed;
    SetActorTickEnabled(false);
    RegisterToGrid();

    OnRep_BuildingState();
    OnConstructionCompleted();
}

void ARTSBuilding::CancelConstruction(bool bRefundResources)
{
    if (!HasAuthority())
    {
        return;
    }

    if (bRefundResources
        && BuildingState == ERTSBuildingState::UnderConstruction
        && OwningPlayerState
        && BuildingData)
    {
        OwningPlayerState->AddResources(
            BuildingData->MineralCost,
            BuildingData->VespeneCost
        );
    }

    Destroy();
}

void ARTSBuilding::SetPreviewBuildingMode(bool bPreview)
{
    if (bPreview)
    {
        UnregisterFromGrid();
    }

    bIsPreviewBuilding = bPreview;

    if (bIsPreviewBuilding)
    {
        BuildingState = ERTSBuildingState::Preview;
    }

    TArray<UPrimitiveComponent*> PrimitiveComponents;
    GetComponents<UPrimitiveComponent>(PrimitiveComponents);

    for (UPrimitiveComponent* PrimComp : PrimitiveComponents)
    {
        if (!PrimComp)
        {
            continue;
        }

        if (bIsPreviewBuilding)
        {
            PrimComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            PrimComp->SetGenerateOverlapEvents(false);
            PrimComp->SetCanEverAffectNavigation(false);
        }
        else
        {
            PrimComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            PrimComp->SetGenerateOverlapEvents(true);
            PrimComp->SetCanEverAffectNavigation(true);
        }
    }

    if (bIsPreviewBuilding)
    {
        Tags.AddUnique(TEXT("BuildPreview"));
    }
    else
    {
        Tags.Remove(TEXT("BuildPreview"));
    }
}

void ARTSBuilding::OnRep_BuildingSetup()
{
    RefreshBuildingVisual();

    if (!HasAuthority())
    {
        RegisterToLocalGridIfNeeded();
    }

    SetActorTickEnabled(HasAuthority() && BuildingState == ERTSBuildingState::UnderConstruction);
}

void ARTSBuilding::OnRep_BuildingState()
{
    RefreshBuildingVisual();

    if (BuildingState == ERTSBuildingState::UnderConstruction)
    {
        RegisterToGrid();
    }
    else if (BuildingState == ERTSBuildingState::Completed)
    {
        RegisterToGrid();
        ApplyCompletedGridEffectsIfNeeded();
    }
    else if (BuildingState == ERTSBuildingState::Flying)
    {
        UnregisterFromGrid();
    }
}


void ARTSBuilding::UpdateSelectionDecal()
{
    if (!SelectionDecalComponent)
    {
        return;
    }

    const float FootprintSizeX = FMath::Max(1, GridWidth) * CachedCellSize;
    const float FootprintSizeY = FMath::Max(1, GridHeight) * CachedCellSize;
    const FVector EffectiveDecalSize = SelectionDecalSize.IsNearlyZero()
        ? FVector(32.0f, FootprintSizeX * 1.08f, FootprintSizeY * 1.08f)
        : SelectionDecalSize;

    SelectionDecalComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
    SelectionDecalComponent->SetRelativeLocation(FVector(0.0f, 0.0f, SelectionDecalZOffset));
    SelectionDecalComponent->DecalSize = EffectiveDecalSize;

    if (SelectionDecalMaterial)
    {
        SelectionDecalComponent->SetDecalMaterial(SelectionDecalMaterial);
    }

    const bool bShowSelectionDecal = bIsSelected && SelectionDecalMaterial != nullptr && !bIsPreviewBuilding;
    SelectionDecalComponent->SetVisibility(bShowSelectionDecal, true);
    SelectionDecalComponent->SetHiddenInGame(!bShowSelectionDecal, true);
}

void ARTSBuilding::RefreshBuildingVisual(){
    if (!MeshComponent)
    {
        return;
    }

    if (BuildingData)
    {
        UStaticMesh* DesiredMesh = BuildingData->BuildingStaticMesh
            ? BuildingData->BuildingStaticMesh
            : BuildingData->PreviewStaticMesh;
        if (DesiredMesh)
        {
            MeshComponent->SetStaticMesh(DesiredMesh);
        }

        SelectionDecalMaterial = BuildingData->SelectionDecalMaterial;
        SelectionDecalSize = BuildingData->SelectionDecalSize;
        for (int32 MaterialIndex = 0; MaterialIndex < BuildingData->OverrideMaterials.Num(); ++MaterialIndex)
        {
            if (BuildingData->OverrideMaterials[MaterialIndex])
            {
                MeshComponent->SetMaterial(MaterialIndex, BuildingData->OverrideMaterials[MaterialIndex]);
            }
        }
    }

    if (MeshComponent->GetStaticMesh())
    {
        FitMeshToGridFootprint(CachedCellSize);
    }

    if (HealthComponent && BuildingData)
    {
        HealthComponent->SetMaxHealth(BuildingData->MaxHealth, HasAuthority());
    }

    if (CombatEffectsComponent && BuildingData)
    {
        CombatEffectsComponent->ConfigureDeathEffect(BuildingData->DeathEffect, BuildingData->DeathSound);
    }

    ApplyTeamVisual();
    UpdateSelectionDecal();
}

void ARTSBuilding::RegisterToLocalGridIfNeeded()
{
    RegisterToGrid();
}

void ARTSBuilding::RegisterToGrid()
{
    if (bIsPreviewBuilding)
    {
        return;
    }

    if (bRegisteredOnLocalGrid)
    {
        return;
    }

    if (!BuildingData)
    {
        return;
    }

    if (GridWidth <= 0 || GridHeight <= 0)
    {
        return;
    }

    if (!OwningGridManager)
    {
        OwningGridManager = ResolveGridManager();
    }

    if (!OwningGridManager)
    {
        return;
    }

    if (BuildingState == ERTSBuildingState::Flying)
    {
        return;
    }

    OwningGridManager->OccupyBuildingCells(
        GridOriginCoord,
        GridWidth,
        GridHeight,
        GetUniqueID()
    );

    if (BuildingData->bMustBuildOnVespeneGeyser)
    {
        const FRTSGridCoord CenterCoord = GetFootprintCenterCoord();
        OwningGridManager->SetVespeneOccupied(CenterCoord, true);
    }

    bRegisteredOnLocalGrid = true;
}

void ARTSBuilding::UnregisterFromGrid()
{
    if (bIsPreviewBuilding)
    {
        return;
    }

    if (!bRegisteredOnLocalGrid && !bCompletedGridEffectsApplied)
    {
        return;
    }

    if (!OwningGridManager)
    {
        OwningGridManager = ResolveGridManager();
    }

    if (!OwningGridManager)
    {
        return;
    }

    RemoveCompletedGridEffectsIfNeeded();

    if (bRegisteredOnLocalGrid)
    {
        OwningGridManager->ReleaseBuildingCells(
            GridOriginCoord,
            GridWidth,
            GridHeight
        );

        if (BuildingData && BuildingData->bMustBuildOnVespeneGeyser)
        {
            OwningGridManager->SetVespeneOccupied(GetFootprintCenterCoord(), false);
        }

        bRegisteredOnLocalGrid = false;
    }
}

ARTSGridManager* ARTSBuilding::ResolveGridManager()
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

float ARTSBuilding::GetSyncedServerTimeSeconds() const
{
    if (!GetWorld())
    {
        return 0.0f;
    }

    const AGameStateBase* GameState = GetWorld()->GetGameState<AGameStateBase>();

    if (GameState)
    {
        return GameState->GetServerWorldTimeSeconds();
    }

    return GetWorld()->GetTimeSeconds();
}

void ARTSBuilding::SetOwningGridManager(ARTSGridManager* InGridManager)
{
    OwningGridManager = InGridManager;
}


bool ARTSBuilding::QueueUnitProduction(URTSUnitData* UnitData)
{
    if (!HasAuthority() || !ProductionQueueComponent || !BuildingData || !UnitData)
    {
        return false;
    }

    bool bCanTrainUnit = false;
    for (URTSUnitData* TrainableUnit : BuildingData->TrainableUnits)
    {
        if (TrainableUnit == UnitData)
        {
            bCanTrainUnit = true;
            break;
        }
    }

    if (!bCanTrainUnit)
    {
        return false;
    }

    return ProductionQueueComponent->QueueUnit(UnitData, OwningPlayerState);
}
void ARTSBuilding::SetProductionRallyPoint(const FVector& WorldLocation)
{
    SetProductionRallyPointTarget(WorldLocation, nullptr);
}

void ARTSBuilding::SetProductionRallyPointTarget(const FVector& WorldLocation, ARTSResourceNode* ResourceTarget)
{
    if (!HasAuthority() || !ProductionQueueComponent)
    {
        return;
    }

    ProductionQueueComponent->SetRallyPointTarget(WorldLocation, ResourceTarget);
}
void ARTSBuilding::ClearProductionRallyPoint()
{
    if (!HasAuthority() || !ProductionQueueComponent)
    {
        return;
    }

    ProductionQueueComponent->ClearRallyPoint();
}

void ARTSBuilding::StopAllCommands()
{
    if (!HasAuthority())
    {
        return;
    }
}

float ARTSBuilding::GetBuildProgress01() const
{
    if (BuildingState == ERTSBuildingState::Completed)
    {
        return 1.0f;
    }

    if (BuildingState != ERTSBuildingState::UnderConstruction)
    {
        return 0.0f;
    }

    if (BuildTime <= KINDA_SMALL_NUMBER)
    {
        return 1.0f;
    }

    const float Now = GetSyncedServerTimeSeconds();
    const float Elapsed = Now - BuildStartServerTime;

    return FMath::Clamp(Elapsed / BuildTime, 0.0f, 1.0f);
}

void ARTSBuilding::ApplyCompletedGridEffectsIfNeeded()
{
    if (bCompletedGridEffectsApplied
        || !BuildingData
        || BuildingState != ERTSBuildingState::Completed)
    {
        return;
    }

    if (!OwningGridManager)
    {
        OwningGridManager = ResolveGridManager();
    }

    if (!OwningGridManager)
    {
        return;
    }

    const FRTSGridCoord CenterCoord = GetFootprintCenterCoord();

    if (BuildingData->bProvidesPower)
    {
        OwningGridManager->AddPowerInRadius(
            CenterCoord,
            BuildingData->PowerRadiusCells
        );
    }

    if (BuildingData->bProvidesCreep)
    {
        OwningGridManager->AddCreepInRadius(
            CenterCoord,
            BuildingData->CreepRadiusCells
        );
    }

    if (OwningPlayerState && BuildingData->SupplyProvided > 0)
    {
        OwningPlayerState->AddSupplyCap(BuildingData->SupplyProvided);
    }

    bCompletedGridEffectsApplied = true;
}

void ARTSBuilding::RemoveCompletedGridEffectsIfNeeded()
{
    if (!bCompletedGridEffectsApplied || !BuildingData)
    {
        return;
    }

    if (!OwningGridManager)
    {
        OwningGridManager = ResolveGridManager();
    }

    if (!OwningGridManager)
    {
        return;
    }

    const FRTSGridCoord CenterCoord = GetFootprintCenterCoord();

    if (BuildingData->bProvidesPower)
    {
        OwningGridManager->RemovePowerInRadius(
            CenterCoord,
            BuildingData->PowerRadiusCells
        );
    }

    if (OwningPlayerState && BuildingData->SupplyProvided > 0)
    {
        OwningPlayerState->RemoveSupplyCap(BuildingData->SupplyProvided);
    }

    // Creep is additive until the grid stores source counts per cell.
    bCompletedGridEffectsApplied = false;
}

FRTSGridCoord ARTSBuilding::GetFootprintCenterCoord() const
{
    return FRTSGridCoord(
        GridOriginCoord.X + GridWidth / 2,
        GridOriginCoord.Y + GridHeight / 2
    );
}

void ARTSBuilding::OnConstructionCompleted_Implementation()
{
    ApplyCompletedGridEffectsIfNeeded();
}

void ARTSBuilding::OnRep_TeamInfo()
{
    ApplyTeamVisual();
}

void ARTSBuilding::ApplyTeamVisual()
{
    if (!bApplyTeamColorToMaterials)
    {
        return;
    }

    ApplyTeamVisualToMesh(MeshComponent);
}

void ARTSBuilding::ApplyTeamVisualToMesh(UMeshComponent* TargetMesh)
{
    if (!TargetMesh || !TargetMesh->IsVisible())
    {
        return;
    }

    const int32 MaterialCount = TargetMesh->GetNumMaterials();
    for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
    {
        if (!TargetMesh->GetMaterial(MaterialIndex))
        {
            continue;
        }

        UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(TargetMesh->GetMaterial(MaterialIndex));
        if (!DynamicMaterial)
        {
            DynamicMaterial = TargetMesh->CreateDynamicMaterialInstance(MaterialIndex);
        }

        if (!DynamicMaterial)
        {
            continue;
        }

        DynamicMaterial->SetVectorParameterValue(TeamColorMaterialParameterName, TeamColor);
        DynamicMaterial->SetScalarParameterValue(TeamNumberMaterialParameterName, static_cast<float>(TeamNumber));
    }
}

bool ARTSBuilding::CanReceiveCommandsFrom(AController* Controller) const
{
    const ARTSPlayerState* PlayerState = Controller
        ? Controller->GetPlayerState<ARTSPlayerState>()
        : nullptr;

    return PlayerState && TeamNumber == PlayerState->TeamNumber;
}

bool ARTSBuilding::CanBeSelectedBy_Implementation(ARTSPlayerController* SelectingController) const
{
    return !bIsPreviewBuilding;
}

void ARTSBuilding::SetSelectionState_Implementation(bool bSelected)
{
    bIsSelected = bSelected;
    UpdateSelectionDecal();
}
bool ARTSBuilding::IsSelected_Implementation() const
{
    return bIsSelected;
}

int32 ARTSBuilding::GetSelectableTeamNumber_Implementation() const
{
    return TeamNumber;
}

bool ARTSBuilding::IsOwnedByPlayerState_Implementation(ARTSPlayerState* PlayerState) const
{
    return PlayerState && TeamNumber == PlayerState->TeamNumber;
}

void ARTSBuilding::SetTeamInfo(int32 NewTeamNumber, const FLinearColor& NewTeamColor)
{
    if (!HasAuthority())
    {
        return;
    }

    TeamNumber = NewTeamNumber;
    TeamColor = NewTeamColor;

    // 서버 자신도 바로 시각 적용
    OnRep_TeamInfo();
}
