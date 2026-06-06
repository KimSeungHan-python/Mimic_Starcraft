// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/RTSGameModeBase.h"
#include "Buildings/RTSStartCamp.h"
#include "Core/RTSPlayerState.h"
#include "Data/RTSRaceStartData.h"
#include "Data/RTSBuildingData.h"
#include "Data/RTSUnitData.h"
#include "Core/RTSPlayerController.h"
#include "Core/RTSHUD.h"
#include "Grid/RTSGridManager.h"
#include "Units/RTSUnitBase.h"
#include "Buildings/RTSBuilding.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

ARTSGameModeBase::ARTSGameModeBase()
{
	PlayerStateClass = ARTSPlayerState::StaticClass();
	HUDClass = ARTSHUD::StaticClass();

	TeamColors =
	{
		FLinearColor::Red,
		FLinearColor::Blue,
		FLinearColor::Green,
		FLinearColor::Yellow
	};
}

void ARTSGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	CollectStartCampsIfNeeded();
}

void ARTSGameModeBase::CollectStartCampsIfNeeded()
{
	if (StartCamps.Num() > 0)
	{
		return;
	}

	for (TActorIterator<ARTSStartCamp> It(GetWorld()); It; ++It)
	{
		ARTSStartCamp* Camp = *It;
		if (!Camp)
		{
			continue;
		}

		StartCamps.Add(Camp);
	}

	StartCamps.Sort([](const ARTSStartCamp& A, const ARTSStartCamp& B)
		{
			return A.CampIndex < B.CampIndex;
		});
}

ARTSStartCamp* ARTSGameModeBase::AssignStartCamp(AController* Controller)
{
	if (!Controller)
	{
		return nullptr;
	}

	if (TObjectPtr<ARTSStartCamp>* ExistingCamp = AssignedCampMap.Find(Controller))
	{
		return ExistingCamp->Get();
	}

	CollectStartCampsIfNeeded();

	TArray<ARTSStartCamp*> FreeCamps;

	for (ARTSStartCamp* Camp : StartCamps)
	{
		if (Camp && !Camp->bOccupied)
		{
			FreeCamps.Add(Camp);
		}
	}

	if (FreeCamps.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No free StartCamp available."));
		return nullptr;
	}

	const int32 RandomIndex = FMath::RandRange(0, FreeCamps.Num() - 1);
	ARTSStartCamp* SelectedCamp = FreeCamps[RandomIndex];

	SelectedCamp->bOccupied = true;
	AssignedCampMap.Add(Controller, SelectedCamp);

	return SelectedCamp;
}

AActor* ARTSGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	if (ARTSStartCamp* Camp = AssignStartCamp(Player))
	{
		return Camp;
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void ARTSGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (!NewPlayer)
	{
		return;
	}

	ARTSStartCamp* Camp = AssignStartCamp(NewPlayer);
	if (!Camp)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to assign StartCamp."));
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
		return;
	}

	InitializePlayerState(NewPlayer, Camp);

	// DefaultPawnClass를 RTSCameraPawn으로 해두면 여기서 Camp 위치에 카메라 Pawn이 스폰됨.
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	SpawnInitialBaseAndWorkers(NewPlayer, Camp);

	if (ARTSPlayerController* RTSPC = Cast<ARTSPlayerController>(NewPlayer))
	{
		RTSPC->Client_SetStartCamera(Camp->GetCameraWorldTransform());
	}
}

void ARTSGameModeBase::InitializePlayerState(APlayerController* NewPlayer, ARTSStartCamp* Camp)
{
	ARTSPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ARTSPlayerState>() : nullptr;
	if (!PS || !Camp)
	{
		return;
	}

	const int32 TeamNumber = PS->GetPlayerId();

	const FLinearColor TeamColor = TeamColors.IsValidIndex(TeamNumber)
		? TeamColors[TeamNumber]
		: FLinearColor::White;

	const ERTSRace Race = GetRaceForPlayer(NewPlayer);

	PS->SetTeamInfo(TeamNumber, TeamColor, Race, Camp->CampIndex);

	if (URTSRaceStartData* StartData = GetStartData(Race))
	{
		PS->SetResources(StartData->InitialMinerals, StartData->InitialVespene);
		PS->SetSupplyCap(StartData->InitialSupplyCap);
	}
}

void ARTSGameModeBase::SpawnInitialBaseAndWorkers(APlayerController* NewPlayer, ARTSStartCamp* Camp)
{
	if (!NewPlayer || !Camp)
	{
		return;
	}

	ARTSPlayerState* PS = NewPlayer->GetPlayerState<ARTSPlayerState>();
	if (!PS)
	{
		return;
	}

	URTSRaceStartData* StartData = GetStartData(PS->Race);
	if (!StartData)
	{
		UE_LOG(LogTemp, Error, TEXT("RaceStartData is missing."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ARTSGridManager* GridManager = Camp->GridManager;
	if (!GridManager)
	{
		for (TActorIterator<ARTSGridManager> It(World); It; ++It)
		{
			GridManager = *It;
			break;
		}
	}

	URTSBuildingData* MainBaseData = StartData->MainBaseBuildingData;
	TSubclassOf<ARTSBuilding> MainBaseClass = MainBaseData && MainBaseData->BuildingClass
		? MainBaseData->BuildingClass
		: StartData->MainBaseClass;

	FTransform BaseTransform = Camp->GetMainBaseWorldTransform();
	FRTSGridCoord BaseOriginCoord = Camp->AnchorCellCoord + Camp->MainBaseOffsetCellCoord;
	int32 BaseWidth = 1;
	int32 BaseHeight = 1;

	if (MainBaseData && GridManager)
	{
		BaseWidth = FMath::Max(1, MainBaseData->GridWidth);
		BaseHeight = FMath::Max(1, MainBaseData->GridHeight);

		const FRTSGridCoord BaseCenterCoord = Camp->AnchorCellCoord + Camp->MainBaseOffsetCellCoord;
		BaseOriginCoord = FRTSGridCoord(
			BaseCenterCoord.X - BaseWidth / 2,
			BaseCenterCoord.Y - BaseHeight / 2
		);

		FVector BaseLocation;
		if (GridManager->GetBuildingCenterLocationOnGround(BaseOriginCoord, BaseWidth, BaseHeight, BaseLocation))
		{
			BaseTransform.SetLocation(BaseLocation);
		}
	}

	ARTSBuilding* MainBase = nullptr;
	if (MainBaseClass)
	{
		MainBase = World->SpawnActorDeferred<ARTSBuilding>(
			MainBaseClass,
			BaseTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
		);

		if (MainBase)
		{
			MainBase->TeamNumber = PS->TeamNumber;
			MainBase->TeamColor = PS->TeamColor;
			MainBase->OwningPlayerState = PS;

			if (MainBaseData && GridManager)
			{
				MainBase->InitializeBuilding(
					MainBaseData,
					BaseOriginCoord,
					BaseWidth,
					BaseHeight,
					GridManager->CellSize,
					GridManager
				);
			}

			UGameplayStatics::FinishSpawningActor(MainBase, BaseTransform);

			if (MainBaseData && GridManager)
			{
				MainBase->SetPreviewBuildingMode(false);
				MainBase->CompleteConstruction();
			}
		}
	}

	TSubclassOf<ARTSUnitBase> WorkerClass = StartData->WorkerClass;
    if (!WorkerClass && StartData->WorkerUnitData)
    {
        WorkerClass = StartData->WorkerUnitData->UnitClass;
    }

    const int32 WorkerSupplyCost = StartData->WorkerUnitData
        ? StartData->WorkerUnitData->SupplyCost
        : StartData->InitialWorkerSupplyCost;

    const int32 WorkerCount = FMath::Min(StartData->InitialWorkerCount, Camp->WorkerLocalTransforms.Num());

	for (int32 i = 0; i < WorkerCount; ++i)
	{
		if (!WorkerClass)
		{
			continue;
		}

		const FTransform WorkerTransform = Camp->GetWorkerWorldTransform(i);

		ARTSUnitBase* Worker = World->SpawnActorDeferred<ARTSUnitBase>(
			WorkerClass,
			WorkerTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
		);

		if (Worker)
		{
			Worker->TeamNumber = PS->TeamNumber;
			Worker->TeamColor = PS->TeamColor;
			Worker->OwningPlayerState = PS;
			Worker->SetUnitData(StartData->WorkerUnitData);
			Worker->RegisterSupplyCost(WorkerSupplyCost, false);

			UGameplayStatics::FinishSpawningActor(Worker, WorkerTransform);
		}
	}

	ApplyRaceStartEffect(NewPlayer, Camp, StartData);
}

void ARTSGameModeBase::ApplyRaceStartEffect(APlayerController* NewPlayer, ARTSStartCamp* Camp, URTSRaceStartData* StartData)
{
	if (!NewPlayer || !Camp || !StartData)
	{
		return;
	}

	ARTSPlayerState* PS = NewPlayer->GetPlayerState<ARTSPlayerState>();
	if (!PS)
	{
		return;
	}

	switch (PS->Race)
	{
	case ERTSRace::Zerg:
	{
		const FTransform CreepTransform = Camp->GetCreepCenterWorldTransform();

		if (StartData->InitialCreepActorClass)
		{
			GetWorld()->SpawnActor<AActor>(
				StartData->InitialCreepActorClass,
				CreepTransform
			);
		}

		if (Camp->GridManager)
		{
			const float CellSize = FMath::Max(KINDA_SMALL_NUMBER, Camp->GridManager->CellSize);
			const int32 RadiusCells = FMath::Max(0, FMath::CeilToInt(Camp->InitialCreepRadius / CellSize));
			const FRTSGridCoord CreepCenterCoord = Camp->GridManager->WorldToGrid(CreepTransform.GetLocation());

			Camp->GridManager->AddCreepInRadius(CreepCenterCoord, RadiusCells);
		}
		break;
	}

	case ERTSRace::Protoss:
	{
		// 시작 Nexus 주변 기본 Power Field가 필요하다면 여기서 생성.
		// AddPowerInRadius(...);
		break;
	}

	case ERTSRace::Terran:
	{
		// 테란은 보통 특별한 지형 필드 없음.
		// 나중에 Command Center Lift 관련 초기 상태만 세팅.
		break;
	}

	default:
		break;
	}
}

ERTSRace ARTSGameModeBase::GetRaceForPlayer(APlayerController* NewPlayer) const
{
	// 임시 기본값.
	// 나중에는 로비 선택값, GameInstance, URL Option, Session Setting 등에서 가져오면 됨.
	return ERTSRace::Zerg;
}

URTSRaceStartData* ARTSGameModeBase::GetStartData(ERTSRace Race) const
{
	if (const TObjectPtr<URTSRaceStartData>* Found = RaceStartDataMap.Find(Race))
	{
		return Found->Get();
	}

	return nullptr;
}
