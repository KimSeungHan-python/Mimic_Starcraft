#include "RTSBuilding.h"
#include "RTSBuildingData.h"
#include "Components/StaticMeshComponent.h"
#include "RTSGridManager.h"

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
}

void ARTSBuilding::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

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

void ARTSBuilding::InitializeBuilding(
    URTSBuildingData* InBuildingData,
    FRTSGridCoord InGridOriginCoord,
    int32 InGridWidth,
    int32 InGridHeight
)
{
    BuildingData = InBuildingData;
    GridOriginCoord = InGridOriginCoord;
    GridWidth = InGridWidth;
    GridHeight = InGridHeight;
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

    UE_LOG(LogTemp, Warning, TEXT("MeshBounds Origin: %s"),
        *MeshBounds.Origin.ToString());

    UE_LOG(LogTemp, Warning, TEXT("MeshBounds Extent: %s"),
        *MeshBounds.BoxExtent.ToString());

    UE_LOG(LogTemp, Warning, TEXT("Mesh NewScale: %s"),
        *NewScale.ToString());

    UE_LOG(LogTemp, Warning, TEXT("Mesh NewRelativeLocation: %s"),
        *NewRelativeLocation.ToString());
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

void ARTSBuilding::SetOwningGridManager(ARTSGridManager* InGridManager)
{
    OwningGridManager = InGridManager;
}

void ARTSBuilding::BeginConstruction(float InBuildTime)
{
    BuildTime = FMath::Max(0.01f, InBuildTime);
    CurrentBuildTime = 0.0f;
    bIsCompleted = false;
    BuildingState = ERTSBuildingState::UnderConstruction;

    // 건설 중에는 기능 비활성화.
    // 공격, 생산, 업그레이드 등은 bIsCompleted 체크해서 막으면 됨.
}

void ARTSBuilding::CompleteConstruction()
{
    CurrentBuildTime = BuildTime;
    bIsCompleted = true;
    BuildingState = ERTSBuildingState::Completed;

    OnConstructionCompleted();
}

float ARTSBuilding::GetBuildProgress01() const
{
    if (BuildTime <= KINDA_SMALL_NUMBER)
    {
        return 1.0f;
    }

    return FMath::Clamp(CurrentBuildTime / BuildTime, 0.0f, 1.0f);
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