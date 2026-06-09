#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTSHealthComponent.generated.h"

class URTSHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRTSHealthChanged, URTSHealthComponent*, HealthComponent, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRTSDeath, URTSHealthComponent*, HealthComponent, AActor*, DamageCauser);

UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class MIMIC_STARCRAFT_API URTSHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URTSHealthComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Health")
    bool bDestroyOwnerOnDeath = true;

    UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "RTS Health")
    float MaxHealth = 100.0f;

    UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "RTS Health")
    float CurrentHealth = 100.0f;

    UPROPERTY(ReplicatedUsing = OnRep_Dead, BlueprintReadOnly, Category = "RTS Health")
    bool bDead = false;

    UPROPERTY(BlueprintAssignable, Category = "RTS Health")
    FRTSHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "RTS Health")
    FRTSDeath OnDeath;

    UFUNCTION(BlueprintCallable, Category = "RTS Health")
    void SetMaxHealth(float NewMaxHealth, bool bResetCurrentHealth);

    UFUNCTION(BlueprintCallable, Category = "RTS Health")
    bool ApplyDamage(float DamageAmount, AActor* DamageCauser);

    UFUNCTION(BlueprintCallable, Category = "RTS Health")
    void Heal(float HealAmount);

    UFUNCTION(BlueprintPure, Category = "RTS Health")
    bool IsAlive() const { return !bDead && CurrentHealth > 0.0f; }

    UFUNCTION(BlueprintPure, Category = "RTS Health")
    float GetHealthFraction() const;

protected:
    UFUNCTION()
    void OnRep_Health();

    UFUNCTION()
    void OnRep_Dead();

    void Die(AActor* DamageCauser);
};
