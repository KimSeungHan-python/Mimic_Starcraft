#include "Core/RTSPlayerController.h"
#include "Grid/RTSGridManager.h"
#include "Buildings/RTSBuilding.h"
#include "Data/RTSBuildingData.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Buildings/RTSBuildGridPreview.h"

ARTSPlayerController::ARTSPlayerController()
{
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
}

void ARTSPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!GridManager)
    {
        GridManager = ResolveGridManager();
    }
}

void ARTSPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (bIsInBuildMode)
    {
        UpdateBuildingPreview();
    }
}

void ARTSPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAction("ConfirmBuild", IE_Pressed, this, &ARTSPlayerController::ConfirmBuild);
    InputComponent->BindAction("CancelBuild", IE_Pressed, this, &ARTSPlayerController::CancelBuildMode);
}

void ARTSPlayerController::StartBuildMode(URTSBuildingData* BuildingData)
{
    if (!BuildingData)
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
    //if (!bIsInBuildMode)
    //{
    //    return;
    //}

    //if (!GridManager || !SelectedBuildingData)
    //{
    //    return;
    //}

    //if (!bCurrentPlacementValid)
    //{
    //    return;
    //}

    //if (!PreviewBuildingActor || !bHasValidPreviewTransform)
    //{
    //    return;
    //}


    //TSubclassOf<ARTSBuilding> BuildingClass = SelectedBuildingData->BuildingClass;

    //if (!BuildingClass)
    //{
    //    BuildingClass = DefaultBuildingClass;
    //}

    //if (!BuildingClass)
    //{
    //    return;
    //}

    ////FVector SpawnBuildingLocation;

    ////GridManager->GetBuildingCenterLocationOnGround(
    ////    CurrentPreviewCoord,
    ////    SelectedBuildingData->GridWidth,
    ////    SelectedBuildingData->GridHeight,
    ////    SpawnBuildingLocation
    ////);

    //FTransform SpawnTransform = LastPreviewTransform;

    //UE_LOG(LogTemp, Warning, TEXT("Spawn from Preview Transform Location: %s"),
    //    *SpawnTransform.GetLocation().ToString()
    //);

    //DrawDebugSphere(
    //    GetWorld(),
    //    SpawnTransform.GetLocation(),
    //    80.0f,
    //    16,
    //    FColor::Yellow,
    //    false,
    //    5.0f
    //);

    //FActorSpawnParameters SpawnParams;
    //SpawnParams.Owner = this;
    //SpawnParams.SpawnCollisionHandlingOverride =
    //    ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    //ARTSBuilding* NewBuilding = GetWorld()->SpawnActor<ARTSBuilding>(
    //    BuildingClass,
    //    SpawnTransform,
    //    SpawnParams
    //);

    //if (!NewBuilding)
    //{
    //    return;
    //}

    //NewBuilding->InitializeBuilding(
    //    SelectedBuildingData,
    //    CurrentPreviewCoord,
    //    SelectedBuildingData->GridWidth,
    //    SelectedBuildingData->GridHeight
    //);

    //NewBuilding->FitMeshToGridFootprint(GridManager->CellSize);

    //NewBuilding->SetPreviewBuildingMode(false);

    //GridManager->OccupyBuildingCells(
    //    CurrentPreviewCoord,
    //    SelectedBuildingData->GridWidth,
    //    SelectedBuildingData->GridHeight,
    //    NewBuilding->GetUniqueID()
    //);

    //// 베스핀 건물이라면 가스 점유 처리
    //if (SelectedBuildingData->bMustBuildOnVespeneGeyser)
    //{
    //    const FRTSGridCoord CenterCoord(
    //        CurrentPreviewCoord.X + SelectedBuildingData->GridWidth / 2,
    //        CurrentPreviewCoord.Y + SelectedBuildingData->GridHeight / 2
    //    );

    //    GridManager->SetVespeneOccupied(CenterCoord, true);
    //}

    //// 바로 완성하지 않고 건설 시작
    //NewBuilding->BeginConstruction(SelectedBuildingData->BuildTime);
    ////UE_LOG(LogTemp, Warning, TEXT("SpawnLocation: %s"), *SpawnBuildingLocation.ToString());
    //CancelBuildMode();
}

void ARTSPlayerController::ServerConfirmBuild_Implementation(FName BuildingId, FRTSGridCoord OriginCoord)
{
    BuildOnServer(BuildingId, OriginCoord);
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

    ARTSBuilding* NewBuilding = GetWorld()->SpawnActor<ARTSBuilding>(
        BuildingClass,
        SpawnTransform,
        SpawnParams
    );

    if (!NewBuilding)
    {
        UE_LOG(LogTemp, Error, TEXT("BuildOnServer failed: SpawnActor returned null"));
        return;
    }

    NewBuilding->InitializeBuilding(
        BuildingData,
        OriginCoord,
        BuildingData->GridWidth,
        BuildingData->GridHeight,
        GridManager->CellSize,
        GridManager
    );

    NewBuilding->SetPreviewBuildingMode(false);
 

    GridManager->OccupyBuildingCells(
        OriginCoord,
        BuildingData->GridWidth,
        BuildingData->GridHeight,
        NewBuilding->GetUniqueID()
    );

    NewBuilding->BeginConstruction(BuildingData->BuildTime);

    UE_LOG(LogTemp, Warning, TEXT("Server spawned building: %s"), *NewBuilding->GetName());
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

    PreviewBuildingActor->SetReplicates(false);
    PreviewBuildingActor->SetReplicateMovement(false);
    PreviewBuildingActor->SetPreviewBuildingMode(true);

    if (!PreviewBuildingActor)
    {
        UE_LOG(LogTemp, Error, TEXT("CreatePreviewActor failed: Spawn failed"));
        return;
    }

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