#include "Buildings/RTSBuilding.h"
#include "Data/RTSBuildingData.h"
#include "Grid/RTSGridManager.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

ARTSBuilding::ARTSBuilding()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(SceneRoot);

    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
    MeshComponent->SetCanEverAffectNavigation(true);
}

void ARTSBuilding::BeginPlay()
{
    Super::BeginPlay();

    if (!OwningGridManager)
    {
        OwningGridManager = ResolveGridManager();
    }

    RefreshBuildingVisual();

    if (!HasAuthority())
    {
        RegisterToLocalGridIfNeeded();
    }
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
    BuildStartServerTime = GetSyncedServerTimeSeconds();
    BuildingState = ERTSBuildingState::UnderConstruction;
    bIsCompleted = false;

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

    OnRep_BuildingState();
    OnConstructionCompleted();
}

void ARTSBuilding::SetPreviewBuildingMode(bool bPreview)
{
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
}

void ARTSBuilding::OnRep_BuildingState()
{
    RefreshBuildingVisual();
}


void ARTSBuilding::RefreshBuildingVisual()
{
    if (!MeshComponent)
    {
        return;
    }

    if (BuildingData && BuildingData->PreviewStaticMesh && !MeshComponent->GetStaticMesh())
    {
        MeshComponent->SetStaticMesh(BuildingData->PreviewStaticMesh);
    }

    if (MeshComponent->GetStaticMesh())
    {
        FitMeshToGridFootprint(CachedCellSize);
    }
}

void ARTSBuilding::RegisterToLocalGridIfNeeded()
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

    bRegisteredOnLocalGrid = true;
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

void ARTSBuilding::OnConstructionCompleted_Implementation()
{
    if (!BuildingData || !OwningGridManager)
    {
        return;
    }

    const FRTSGridCoord CenterCoord(
        GridOriginCoord.X + GridWidth / 2,
        GridOriginCoord.Y + GridHeight / 2
    );

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
    // Blueprint에서 완성 이펙트, 사운드, UI 업데이트 가능
}

void ARTSBuilding::OnRep_TeamInfo()
{
    ApplyTeamVisual();
}

void ARTSBuilding::ApplyTeamVisual()
{
    // 건물 머티리얼 색상 변경 처리
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