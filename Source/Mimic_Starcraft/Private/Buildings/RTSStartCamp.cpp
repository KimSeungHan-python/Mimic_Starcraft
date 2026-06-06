// Fill out your copyright notice in the Description page of Project Settings.


#include "Buildings/RTSStartCamp.h"
#include "Grid/RTSGridManager.h"

ARTSStartCamp::ARTSStartCamp(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = false;
}

void ARTSStartCamp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!GridManager)
	{
		return;
	}
	//해당 cell의 x,y가져옴
	AnchorCellCoord = GridManager->WorldToGrid(GetActorLocation());

	if (bSnapToGridInEditor)
	{
		const FVector SnappedLocation = GridManager->GridToWorldCenter(AnchorCellCoord);
		SetActorLocation(SnappedLocation);
	}
}

FTransform ARTSStartCamp::GetMainBaseWorldTransform() const
{
	if (!GridManager)
	{
		return GetActorTransform();
	}

	const FRTSGridCoord BaseCellCoord = AnchorCellCoord + MainBaseOffsetCellCoord;
	const FVector BaseLocation = GridManager->GridToWorldCenter(BaseCellCoord);
	const FRotator BaseRotation = GetRotationFacingMinerals(BaseLocation);

	return FTransform(BaseRotation, BaseLocation);
}

FTransform ARTSStartCamp::GetWorkerWorldTransform(int32 Index) const
{
	if (!GridManager || !WorkerOffsetCells.IsValidIndex(Index))
	{
		return GetActorTransform();
	}

	const FRTSGridCoord WorkerCellCoord = AnchorCellCoord + WorkerOffsetCells[Index];
	const FVector WorkerLocation = GridManager->GridToWorldCenter(WorkerCellCoord);

	const FRotator WorkerRotation = GetRotationFacingMinerals(WorkerLocation);

	return FTransform(WorkerRotation, WorkerLocation);
}

FTransform ARTSStartCamp::GetCameraWorldTransform() const
{
    FVector FocusLocation = GetMainBaseWorldTransform().GetLocation();
    FocusLocation.X += CameraOffset.X;
    FocusLocation.Y += CameraOffset.Y;

    if (GridManager)
    {
        const FRTSGridCoord FocusCoord = GridManager->WorldToGrid(FocusLocation);
        FVector GroundLocation;
        if (GridManager->GetCellWorldCenterOnGround(FocusCoord, GroundLocation))
        {
            FocusLocation.Z = GroundLocation.Z;
        }
    }

    FVector LookTarget = GetMainBaseWorldTransform().GetLocation();
    if (MineralCenterActor)
    {
        LookTarget = MineralCenterActor->GetActorLocation();
    }

    FVector ToTarget = LookTarget - FocusLocation;
    ToTarget.Z = 0.f;

    FRotator CameraPawnRotation = GetActorRotation();
    if (!ToTarget.IsNearlyZero())
    {
        CameraPawnRotation = FRotationMatrix::MakeFromX(ToTarget).Rotator();
        CameraPawnRotation.Pitch = 0.f;
        CameraPawnRotation.Roll = 0.f;
    }

    return FTransform(CameraPawnRotation, FocusLocation);
}

FTransform ARTSStartCamp::GetCreepCenterWorldTransform() const
{
	return CreepCenterLocalTransform * GetActorTransform();
}

FRotator ARTSStartCamp::GetRotationFacingMinerals(const FVector& FromLocation) const
{
	if (!MineralCenterActor)
	{
		return GetActorRotation();
	}

	FVector ToMineral = MineralCenterActor->GetActorLocation() - FromLocation;
	ToMineral.Z = 0.f;

	if (ToMineral.IsNearlyZero())
	{
		return GetActorRotation();
	}

	const FRotator LookAtRotation = FRotationMatrix::MakeFromX(ToMineral).Rotator();

	return FRotator(
		0.f,
		LookAtRotation.Yaw + MainBaseYawOffset,
		0.f
	);
}
