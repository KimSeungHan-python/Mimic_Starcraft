// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/RTSPlayerState.h"
#include "Net/UnrealNetwork.h"
ARTSPlayerState::ARTSPlayerState()
{
	bReplicates = true;
}

// 게임 서버에서 보내줌 
void ARTSPlayerState::SetTeamInfo(int32 InTeamNumber, const FLinearColor& InTeamColor, ERTSRace InRace, int32 InCampIndex)
{
	TeamNumber = InTeamNumber;
	TeamColor = InTeamColor;
	Race = InRace;
	AssignedCampIndex = InCampIndex;

	OnRep_TeamInfo();
}

void ARTSPlayerState::OnRep_TeamInfo()
{
	// 여기서 UI 색상 갱신, 미니맵 색상 갱신, 선택 테두리 색상 갱신 등을 호출하면 됨.
}

void ARTSPlayerState::SetResources(int32 InMinerals, int32 InVespene)
{
	if (!HasAuthority())
	{
		return;
	}

	Minerals = FMath::Max(0, InMinerals);
	Vespene = FMath::Max(0, InVespene);

	OnRep_Resources();
}

void ARTSPlayerState::SetLobbySelection(ERTSRace InRace, const FLinearColor& InColor)
{
	if (!HasAuthority())
	{
		return;
	}

	LobbySelectedRace = InRace;
	LobbySelectedColor = InColor;

	OnRep_LobbySelection();
}

void ARTSPlayerState::SetRoomHost(bool bInIsHost)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsRoomHost = bInIsHost;
}

void ARTSPlayerState::OnRep_LobbySelection()
{
	// 나중에 UI 갱신용 Delegate 넣어도 됨
}

void ARTSPlayerState::OnRep_Resources()
{
}

void ARTSPlayerState::OnRep_Supply()
{
}

bool ARTSPlayerState::CanAfford(int32 MineralCost, int32 VespeneCost) const
{
	return Minerals >= FMath::Max(0, MineralCost)
		&& Vespene >= FMath::Max(0, VespeneCost);
}

bool ARTSPlayerState::TrySpendResources(int32 MineralCost, int32 VespeneCost)
{
	if (!HasAuthority() || !CanAfford(MineralCost, VespeneCost))
	{
		return false;
	}

	Minerals -= FMath::Max(0, MineralCost);
	Vespene -= FMath::Max(0, VespeneCost);

	OnRep_Resources();
	return true;
}

bool ARTSPlayerState::CanReserveSupply(int32 SupplyCost) const
{
	const int32 ClampedCost = FMath::Max(0, SupplyCost);
	return SupplyUsed + ClampedCost <= SupplyCap;
}

bool ARTSPlayerState::TryReserveSupply(int32 SupplyCost)
{
	if (!HasAuthority() || !CanReserveSupply(SupplyCost))
	{
		return false;
	}

	SupplyUsed += FMath::Max(0, SupplyCost);
	OnRep_Supply();
	return true;
}

void ARTSPlayerState::ReleaseSupply(int32 SupplyAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	SupplyUsed = FMath::Max(0, SupplyUsed - FMath::Max(0, SupplyAmount));
	OnRep_Supply();
}

void ARTSPlayerState::AddSupplyCap(int32 SupplyAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	SupplyCap = FMath::Clamp(SupplyCap + FMath::Max(0, SupplyAmount), 0, MaxSupplyCap);
	OnRep_Supply();
}

void ARTSPlayerState::RemoveSupplyCap(int32 SupplyAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	SupplyCap = FMath::Clamp(SupplyCap - FMath::Max(0, SupplyAmount), 0, MaxSupplyCap);
	OnRep_Supply();
}

void ARTSPlayerState::SetSupplyCap(int32 InSupplyCap)
{
	if (!HasAuthority())
	{
		return;
	}

	SupplyCap = FMath::Clamp(InSupplyCap, 0, MaxSupplyCap);
	SupplyUsed = FMath::Min(SupplyUsed, SupplyCap);
	OnRep_Supply();
}

void ARTSPlayerState::AddResources(int32 MineralAmount, int32 VespeneAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	Minerals = FMath::Max(0, Minerals + MineralAmount);
	Vespene = FMath::Max(0, Vespene + VespeneAmount);

	OnRep_Resources();
}

void ARTSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARTSPlayerState, TeamNumber);
	DOREPLIFETIME(ARTSPlayerState, TeamColor);
	DOREPLIFETIME(ARTSPlayerState, Race);
	DOREPLIFETIME(ARTSPlayerState, AssignedCampIndex);
	DOREPLIFETIME(ARTSPlayerState, Minerals);
	DOREPLIFETIME(ARTSPlayerState, Vespene);
	DOREPLIFETIME(ARTSPlayerState, SupplyUsed);
	DOREPLIFETIME(ARTSPlayerState, SupplyCap);
	DOREPLIFETIME(ARTSPlayerState, LobbySelectedRace);
	DOREPLIFETIME(ARTSPlayerState, LobbySelectedColor);
	DOREPLIFETIME(ARTSPlayerState, bIsRoomHost);
}
