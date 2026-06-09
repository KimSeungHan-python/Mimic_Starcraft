#include "Components/RTSHealthComponent.h"

#include "Net/UnrealNetwork.h"

URTSHealthComponent::URTSHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void URTSHealthComponent::BeginPlay()
{
    Super::BeginPlay();

    MaxHealth = FMath::Max(1.0f, MaxHealth);
    CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
    bDead = CurrentHealth <= 0.0f;
}

void URTSHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(URTSHealthComponent, MaxHealth);
    DOREPLIFETIME(URTSHealthComponent, CurrentHealth);
    DOREPLIFETIME(URTSHealthComponent, bDead);
}

void URTSHealthComponent::SetMaxHealth(float NewMaxHealth, bool bResetCurrentHealth)
{
    MaxHealth = FMath::Max(1.0f, NewMaxHealth);

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        if (bResetCurrentHealth)
        {
            CurrentHealth = MaxHealth;
            bDead = false;
        }
        else
        {
            CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);
            bDead = CurrentHealth <= 0.0f;
        }
    }

    OnRep_Health();
}

bool URTSHealthComponent::ApplyDamage(float DamageAmount, AActor* DamageCauser)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !IsAlive())
    {
        return false;
    }

    const float ClampedDamage = FMath::Max(0.0f, DamageAmount);
    if (ClampedDamage <= 0.0f)
    {
        return false;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth - ClampedDamage, 0.0f, MaxHealth);
    OnRep_Health();

    if (CurrentHealth <= 0.0f)
    {
        Die(DamageCauser);
    }

    return true;
}

void URTSHealthComponent::Heal(float HealAmount)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !IsAlive())
    {
        return;
    }

    CurrentHealth = FMath::Clamp(CurrentHealth + FMath::Max(0.0f, HealAmount), 0.0f, MaxHealth);
    OnRep_Health();
}

float URTSHealthComponent::GetHealthFraction() const
{
    return MaxHealth > KINDA_SMALL_NUMBER
        ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f)
        : 0.0f;
}

void URTSHealthComponent::OnRep_Health()
{
    OnHealthChanged.Broadcast(this, CurrentHealth, MaxHealth);
}

void URTSHealthComponent::OnRep_Dead()
{
    if (bDead)
    {
        OnDeath.Broadcast(this, nullptr);
    }
}

void URTSHealthComponent::Die(AActor* DamageCauser)
{
    if (bDead)
    {
        return;
    }

    bDead = true;
    CurrentHealth = 0.0f;
    OnRep_Health();
    OnDeath.Broadcast(this, DamageCauser);

    if (bDestroyOwnerOnDeath && GetOwner())
    {
        GetOwner()->Destroy();
    }
}
