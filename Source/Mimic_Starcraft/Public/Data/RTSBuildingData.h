#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputCoreTypes.h"
#include "RTSBuildingData.generated.h"

class ARTSBuilding;
class UMaterialInterface;
class UParticleSystem;
class UStaticMesh;
class USoundBase;
class UTexture2D;
class URTSUnitData;

UENUM(BlueprintType)
enum class ERTSBuildingRace : uint8
{
    Neutral,
    Terran,
    Protoss,
    Zerg
};

UCLASS(BlueprintType, Blueprintable)
class MIMIC_STARCRAFT_API URTSBuildingData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    FName BuildingId;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    FText DisplayName;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    ERTSBuildingRace Race = ERTSBuildingRace::Neutral;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Grid")
    int32 GridWidth = 2;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Grid")
    int32 GridHeight = 2;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
    TSubclassOf<ARTSBuilding> BuildingClass;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Command")
    FKey CommandHotkey;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Command")
    TObjectPtr<UTexture2D> CommandIcon = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Selection")
    TObjectPtr<UTexture2D> SelectionIcon = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Selection")
    TObjectPtr<UMaterialInterface> SelectionDecalMaterial = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Selection")
    FVector SelectionDecalSize = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Command")
    FText CommandTooltip;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Command")
    int32 CommandCardOrder = 0;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Command")
    FName WorkerCommandPage = TEXT("Build");


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Visual")
    UStaticMesh* BuildingStaticMesh = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Visual")
    UStaticMesh* PreviewStaticMesh = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Visual")
    TArray<TObjectPtr<UMaterialInterface>> OverrideMaterials;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Production")
    TArray<TObjectPtr<URTSUnitData>> TrainableUnits;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Supply")
    int32 SupplyProvided = 0;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Combat")
    float MaxHealth = 400.0f;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Combat|Effects")
    TObjectPtr<UParticleSystem> DeathEffect = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Combat|Effects")
    TObjectPtr<USoundBase> DeathSound = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Vision", meta = (ClampMin = "0"))
    int32 SightRadiusCells = 10;

    // ê±´ì„¤ ?œê°„
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Construction")
    float BuildTime = 10.0f;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Cost")
    int32 MineralCost = 0;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Cost")
    int32 VespeneCost = 0;

    // ?Œë? ê±´ë¬¼??
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Terran")
    bool bCanLiftOff = false;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Terran")
    float LiftOffHeight = 300.0f;

    // ?„ë¡œ? ìŠ¤ ê±´ë¬¼??
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Protoss")
    bool bRequiresPower = false;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Protoss")
    bool bProvidesPower = false;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Protoss")
    int32 PowerRadiusCells = 6;

    // ?€ê·?ê±´ë¬¼??
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Zerg")
    bool bRequiresCreep = false;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Zerg")
    bool bProvidesCreep = false;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Zerg")
    int32 CreepRadiusCells = 5;

    // ë² ìŠ¤?€ ê±´ë¬¼??
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Resource")
    bool bMustBuildOnVespeneGeyser = false;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Resource")
    bool bAcceptsMineralDropOff = false;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Resource")
    bool bAcceptsVespeneDropOff = false;
};
