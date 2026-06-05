#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Structure/RTSGridTypes.h"
#include "RTSBuilding.generated.h"

class URTSBuildingData;
class ARTSGridManager;
class ARTSPlayerState;

UENUM(BlueprintType)
enum class ERTSBuildingState : uint8
{
    Preview,
    UnderConstruction,
    Completed,
    Flying
};

UCLASS()
class MIMIC_STARCRAFT_API ARTSBuilding : public AActor
{
    GENERATED_BODY()

public:
    ARTSBuilding();

protected:
    virtual void BeginPlay() override;


public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    URTSBuildingData* BuildingData;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    FRTSGridCoord GridOriginCoord;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    int32 GridWidth = 1;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    int32 GridHeight = 1;

    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void InitializeBuilding(
        URTSBuildingData* InBuildingData,
        FRTSGridCoord InGridOriginCoord,
        int32 InGridWidth,
        int32 InGridHeight
    );

    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void FitMeshToGridFootprint(float CellSize);

 
    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void SetPreviewBuildingMode(bool bPreview);

    //건물 추가 Update들 
    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    TObjectPtr<ARTSGridManager> OwningGridManager = nullptr;

    

    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void SetOwningGridManager(ARTSGridManager* InGridManager);


    UPROPERTY(BlueprintReadOnly, Category = "RTS Building|Construction")
    ERTSBuildingState BuildingState = ERTSBuildingState::Completed;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building|Construction")
    float BuildTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building|Construction")
    float CurrentBuildTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building|Construction")
    bool bIsCompleted = true;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building|Preview")
    bool bIsPreviewBuilding = false;

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Construction")
    void BeginConstruction(float InBuildTime);

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Construction")
    void CompleteConstruction();

    UFUNCTION(BlueprintCallable, Category = "RTS Building|Construction")
    float GetBuildProgress01() const;

    UFUNCTION(BlueprintNativeEvent, Category = "RTS Building|Construction")
    void OnConstructionCompleted();

    // Start 및 소유 관련 변수
public:

    UPROPERTY(ReplicatedUsing = OnRep_TeamInfo, BlueprintReadOnly, Category = "RTS Team")
    int32 TeamNumber = -1;

    UPROPERTY(ReplicatedUsing = OnRep_TeamInfo, BlueprintReadOnly, Category = "RTS Team")
    FLinearColor TeamColor = FLinearColor::White;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Team")
    TObjectPtr<ARTSPlayerState> OwningPlayerState;

    UFUNCTION()
    void OnRep_TeamInfo();

    void ApplyTeamVisual();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION()
    void SetTeamInfo(int32 NewTeamNumber, const FLinearColor& NewTeamColor);
};