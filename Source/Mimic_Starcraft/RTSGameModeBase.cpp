// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSGameModeBase.h"
#include "RTSStartCamp.h"
#include "RTSPlayerState.h"
#include "RTSRaceStartData.h"
#include "RTSPlayerController.h"
#include "RTSUnitBase.h"
#include "RTSBuilding.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

ARTSGameModeBase::ARTSGameModeBase()
{
	PlayerStateClass = ARTSPlayerState::StaticClass();

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

	ARTSBuilding* MainBase = nullptr;
	const FTransform BaseTransform = Camp->GetMainBaseWorldTransform();

	if (StartData->MainBaseClass)
	{
		MainBase = World->SpawnActorDeferred<ARTSBuilding>(
			StartData->MainBaseClass,
			BaseTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
		);

		//색깔 가져오기
		if (MainBase)
		{
			MainBase->TeamNumber = PS->TeamNumber;
			MainBase->TeamColor = PS->TeamColor;
			MainBase->OwningPlayerState = PS;

			UGameplayStatics::FinishSpawningActor(MainBase, Camp->GetMainBaseWorldTransform());
		}
	}

	const int32 WorkerCount = FMath::Min(StartData->InitialWorkerCount, Camp->WorkerLocalTransforms.Num());

	for (int32 i = 0; i < WorkerCount; ++i)
	{
		if (!StartData->WorkerClass)
		{
			continue;
		}

		const FTransform WorkerTransform = Camp->GetWorkerWorldTransform(i);

		//일꾼
		ARTSUnitBase* Worker = World->SpawnActorDeferred<ARTSUnitBase>(
			StartData->WorkerClass,
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
		if (StartData->InitialCreepActorClass)
		{
			FTransform CreepTransform = Camp->GetCreepCenterWorldTransform();

			AActor* CreepActor = GetWorld()->SpawnActor<AActor>(
				StartData->InitialCreepActorClass,
				CreepTransform
			);

			// 더 좋은 방식:
			// GridManager 또는 CreepManager가 있다면 여기서
			// AddCreepInRadius(CreepCenter, Camp->InitialCreepRadius, PS->TeamNumber);
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