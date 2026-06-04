// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSUnitBase.h"

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

