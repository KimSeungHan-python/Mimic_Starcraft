#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputCoreTypes.h"
#include "RTSUnitData.generated.h"

class ARTSUnitBase;
class ARTSProjectile;
class URTSBuildingData;
class UAnimMontage;
class UAnimInstance;
class UMaterialInterface;
class UParticleSystem;
class USkeletalMesh;
class USoundBase;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class MIMIC_STARCRAFT_API URTSUnitData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    FName UnitId;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    FText DisplayName;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
    TSubclassOf<ARTSUnitBase> UnitClass;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Visual")
    TObjectPtr<USkeletalMesh> UnitSkeletalMesh = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Visual")
    TSubclassOf<UAnimInstance> AnimationClass;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Visual")
    TArray<TObjectPtr<UMaterialInterface>> OverrideMaterials;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Command")
    FKey CommandHotkey;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Command")
    TObjectPtr<UTexture2D> CommandIcon = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Selection")
    TObjectPtr<UTexture2D> SelectionIcon = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Selection")
    TObjectPtr<UMaterialInterface> SelectionDecalMaterial = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Selection")
    FVector SelectionDecalSize = FVector(32.0f, 120.0f, 120.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Command")
    FText CommandTooltip;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Command")
    int32 CommandCardOrder = 0;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Worker", meta = (EditCondition = "bWorker"))
    FKey BuildMenuHotkey;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Worker", meta = (EditCondition = "bWorker"))
    FText BuildMenuDisplayName;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Worker", meta = (EditCondition = "bWorker"))
    TObjectPtr<UTexture2D> BuildMenuIcon = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Worker", meta = (EditCondition = "bWorker"))
    FName BuildCommandPage = TEXT("Build");


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Production")
    float BuildTime = 12.0f;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Cost")
    int32 MineralCost = 50;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Cost")
    int32 VespeneCost = 0;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Supply")
    int32 SupplyCost = 1;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Movement")
    float MovementSpeed = 550.0f;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Movement")
    float MovementAcceptanceRadius = 45.0f;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat")
    float MaxHealth = 45.0f;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Vision", meta = (ClampMin = "0"))
    int32 SightRadiusCells = 8;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat")
    bool bCanAttack = true;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat", meta = (EditCondition = "bCanAttack"))
    float AttackDamage = 5.0f;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat", meta = (EditCondition = "bCanAttack"))
    float AttackRange = 175.0f;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat", meta = (EditCondition = "bCanAttack"))
    float AttackCooldown = 1.0f;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat|Effects", meta = (EditCondition = "bCanAttack"))
    TSubclassOf<ARTSProjectile> ProjectileClass;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat|Effects", meta = (EditCondition = "bCanAttack"))
    float ProjectileSpeed = 1200.0f;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat|Effects", meta = (EditCondition = "bCanAttack"))
    FName MuzzleSocketName = TEXT("Muzzle");


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat|Effects", meta = (EditCondition = "bCanAttack"))
    TObjectPtr<UAnimMontage> AttackMontage = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat|Effects", meta = (EditCondition = "bCanAttack"))
    TObjectPtr<UParticleSystem> MuzzleEffect = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat|Effects", meta = (EditCondition = "bCanAttack"))
    TObjectPtr<UParticleSystem> ImpactEffect = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat|Effects", meta = (EditCondition = "bCanAttack"))
    TObjectPtr<USoundBase> AttackSound = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat|Effects", meta = (EditCondition = "bCanAttack"))
    TObjectPtr<USoundBase> ImpactSound = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat|Effects")
    TObjectPtr<UParticleSystem> DeathEffect = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Combat|Effects")
    TObjectPtr<USoundBase> DeathSound = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Role")
    bool bWorker = false;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Worker", meta = (EditCondition = "bWorker"))
    TArray<TObjectPtr<URTSBuildingData>> BuildableBuildings;
};
