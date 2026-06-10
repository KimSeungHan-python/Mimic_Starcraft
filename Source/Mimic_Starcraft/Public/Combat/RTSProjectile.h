#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTSProjectile.generated.h"

class UParticleSystem;
class USceneComponent;
class USoundBase;

UCLASS()
class MIMIC_STARCRAFT_API ARTSProjectile : public AActor
{
    GENERATED_BODY()

public:
    ARTSProjectile();

    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Projectile")
    float Speed = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Projectile")
    float ImpactRadius = 35.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Projectile")
    float MaxLifetime = 5.0f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Projectile")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Projectile")
    TObjectPtr<AActor> DamageCauser = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Projectile")
    float Damage = 0.0f;

    UFUNCTION(BlueprintCallable, Category = "RTS Projectile")
    void InitializeProjectile(
        AActor* InTargetActor,
        float InDamage,
        AActor* InDamageCauser,
        float InSpeed,
        UParticleSystem* InImpactEffect,
        USoundBase* InImpactSound
    );

protected:
    UPROPERTY()
    TObjectPtr<UParticleSystem> ImpactEffect = nullptr;

    UPROPERTY()
    TObjectPtr<USoundBase> ImpactSound = nullptr;

    void ImpactTarget();

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastPlayImpactFX(FVector ImpactLocation, UParticleSystem* EffectTemplate, USoundBase* SoundTemplate);
};
