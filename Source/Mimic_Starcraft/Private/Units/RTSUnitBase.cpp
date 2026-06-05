// Fill out your copyright notice in the Description page of Project Settings.


#include "Units/RTSUnitBase.h"
#include "Net/UnrealNetwork.h"
#include "Core/RTSPlayerState.h"

// Sets default values
ARTSUnitBase::ARTSUnitBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARTSUnitBase::BeginPlay()
{
	Super::BeginPlay();

	
}

// Called every frame
void ARTSUnitBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARTSUnitBase::OnRep_TeamInfo()
{
	ApplyTeamVisual();
}

void ARTSUnitBase::ApplyTeamVisual()
{
	// 여기서 머티리얼 색상 변경, 선택 원 색상 변경 등 처리
}

void ARTSUnitBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARTSUnitBase, TeamNumber);
    DOREPLIFETIME(ARTSUnitBase, TeamColor);
}

void ARTSUnitBase::SetTeamInfo(int32 NewTeamNumber, const FLinearColor& NewTeamColor)
{
    if (!HasAuthority())
    {
        return;
    }

    TeamNumber = NewTeamNumber;
    TeamColor = NewTeamColor;

    OnRep_TeamInfo();
}