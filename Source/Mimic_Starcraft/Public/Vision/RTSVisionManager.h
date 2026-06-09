#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/RTSGridTypes.h"
#include "RTSVisionManager.generated.h"

class ARTSBuilding;
class ARTSGridManager;
class ARTSPlayerController;
class ARTSUnitBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRTSVisionUpdated);

struct FRTSTeamVisionState
{
    TSet<int32> VisibleCells;
    TSet<int32> ExploredCells;
};

UCLASS(Blueprintable)
class MIMIC_STARCRAFT_API ARTSVisionManager : public AActor
{
    GENERATED_BODY()

public:
    ARTSVisionManager();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Vision")
    TObjectPtr<ARTSGridManager> GridManager = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Vision")
    float UpdateInterval = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Vision")
    bool bApplyLocalActorVisibility = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Vision")
    bool bDrawDebugLocalVision = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Vision")
    int32 DefaultUnitSightRadiusCells = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Vision")
    int32 DefaultBuildingSightRadiusCells = 10;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Vision")
    int32 LocalTeamNumber = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Vision")
    TArray<FRTSGridCoord> LocalVisibleCells;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Vision")
    TArray<FRTSGridCoord> LocalExploredCells;

    UPROPERTY(BlueprintAssignable, Category = "RTS Vision")
    FRTSVisionUpdated OnLocalVisionUpdated;

    UFUNCTION(BlueprintCallable, Category = "RTS Vision")
    void RefreshVision();

    UFUNCTION(BlueprintPure, Category = "RTS Vision")
    bool IsGridCoordVisibleToTeam(FRTSGridCoord Coord, int32 TeamNumber) const;

    UFUNCTION(BlueprintPure, Category = "RTS Vision")
    bool IsGridCoordExploredByTeam(FRTSGridCoord Coord, int32 TeamNumber) const;

    UFUNCTION(BlueprintPure, Category = "RTS Vision")
    bool IsWorldLocationVisibleToTeam(const FVector& WorldLocation, int32 TeamNumber) const;

    UFUNCTION(BlueprintPure, Category = "RTS Vision")
    bool IsWorldLocationExploredByTeam(const FVector& WorldLocation, int32 TeamNumber) const;

    UFUNCTION(BlueprintPure, Category = "RTS Vision")
    bool IsActorVisibleToTeam(AActor* Actor, int32 TeamNumber) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Vision")
    void ApplyLocalActorVisibility();

private:
    TMap<int32, FRTSTeamVisionState> TeamVisionStates;
    float TimeUntilNextUpdate = 0.0f;

    ARTSGridManager* ResolveGridManager();
    ARTSPlayerController* ResolveLocalPlayerController() const;
    int32 ResolveLocalTeamNumber() const;
    int32 CoordToVisionIndex(FRTSGridCoord Coord) const;
    FRTSGridCoord VisionIndexToCoord(int32 Index) const;
    bool IsVisionIndexValid(int32 Index) const;
    void ResetCurrentVisibleCells();
    void MarkActorVision(AActor* Actor, int32 TeamNumber, int32 RadiusCells);
    void MarkVisionCircle(FRTSGridCoord CenterCoord, int32 RadiusCells, FRTSTeamVisionState& VisionState) const;
    int32 GetUnitSightRadiusCells(const ARTSUnitBase* Unit) const;
    int32 GetBuildingSightRadiusCells(const ARTSBuilding* Building) const;
    int32 GetActorTeamNumber(AActor* Actor) const;
    bool ShouldLocallyShowActor(AActor* Actor, int32 ViewerTeamNumber) const;
    void UpdateLocalVisionCache();
    void DrawDebugLocalVision() const;
};
