#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTSCombatEffectsComponent.generated.h"

class ARTSProjectile;
class UAnimMontage;
class UParticleSystem;
class URTSHealthComponent;
class URTSUnitData;
class USoundBase;

UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class MIMIC_STARCRAFT_API URTSCombatEffectsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URTSCombatEffectsComponent();

    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Combat Effects|Attack")
    TSubclassOf<ARTSProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Combat Effects|Attack")
    float ProjectileSpeed = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Combat Effects|Attack")
    FName MuzzleSocketName = TEXT("Muzzle");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Combat Effects|Attack")
    float MuzzleForwardOffset = 55.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Combat Effects|Attack")
    TObjectPtr<UAnimMontage> AttackMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Combat Effects|Attack")
    TObjectPtr<UParticleSystem> MuzzleEffect = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Combat Effects|Attack")
    TObjectPtr<UParticleSystem> ImpactEffect = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Combat Effects|Attack")
    TObjectPtr<USoundBase> AttackSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Combat Effects|Attack")
    TObjectPtr<USoundBase> ImpactSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Combat Effects|Death")
    TObjectPtr<UParticleSystem> DeathEffect = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Combat Effects|Death")
    TObjectPtr<USoundBase> DeathSound = nullptr;

    UFUNCTION(BlueprintCallable, Category = "RTS Combat Effects")
    void ConfigureFromUnitData(URTSUnitData* UnitData);

    UFUNCTION(BlueprintCallable, Category = "RTS Combat Effects")
    void ConfigureDeathEffect(UParticleSystem* InDeathEffect, USoundBase* InDeathSound);

    UFUNCTION(BlueprintCallable, Category = "RTS Combat Effects")
    bool ExecuteAttack(AActor* TargetActor, float Damage, AActor* DamageCauser);

protected:
    UFUNCTION()
    void HandleOwnerDeath(URTSHealthComponent* HealthComponent, AActor* DamageCauser);

    FTransform GetMuzzleTransform() const;
    void PlayAttackAnimation() const;

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayAttackFX(AActor* TargetActor, FVector MuzzleLocation, FRotator MuzzleRotation);

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayImpactFX(FVector ImpactLocation);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayDeathFX(FVector DeathLocation);
};
