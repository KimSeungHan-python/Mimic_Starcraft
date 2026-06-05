// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSPlayerState.h"
#include "Net/UnrealNetwork.h"
ARTSPlayerState::ARTSPlayerState()
{
	bReplicates = true;
}

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

void ARTSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARTSPlayerState, TeamNumber);
	DOREPLIFETIME(ARTSPlayerState, TeamColor);
	DOREPLIFETIME(ARTSPlayerState, Race);
	DOREPLIFETIME(ARTSPlayerState, AssignedCampIndex);
}