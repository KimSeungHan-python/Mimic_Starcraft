#include "Components/RTSCombatEffectsComponent.h"

#include "Animation/AnimInstance.h"
#include "Combat/RTSProjectile.h"
#include "Components/RTSHealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/RTSUnitData.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

URTSCombatEffectsComponent::URTSCombatEffectsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void URTSCombatEffectsComponent::BeginPlay()
{
    Super::BeginPlay();

    if (URTSHealthComponent* HealthComponent = GetOwner()
        ? GetOwner()->FindComponentByClass<URTSHealthComponent>()
        : nullptr)
    {
        HealthComponent->OnDeath.AddUniqueDynamic(this, &URTSCombatEffectsComponent::HandleOwnerDeath);
    }
}

void URTSCombatEffectsComponent::ConfigureFromUnitData(URTSUnitData* UnitData)
{
    if (!UnitData)
    {
        return;
    }

    ProjectileClass = UnitData->ProjectileClass;
    ProjectileSpeed = UnitData->ProjectileSpeed;
    MuzzleSocketName = UnitData->MuzzleSocketName;
    AttackMontage = UnitData->AttackMontage;
    MuzzleEffect = UnitData->MuzzleEffect;
    ImpactEffect = UnitData->ImpactEffect;
    DeathEffect = UnitData->DeathEffect;
    AttackSound = UnitData->AttackSound;
    ImpactSound = UnitData->ImpactSound;
    DeathSound = UnitData->DeathSound;
}

void URTSCombatEffectsComponent::ConfigureDeathEffect(UParticleSystem* InDeathEffect, USoundBase* InDeathSound)
{
    DeathEffect = InDeathEffect;
    DeathSound = InDeathSound;
}

bool URTSCombatEffectsComponent::ExecuteAttack(AActor* TargetActor, float Damage, AActor* DamageCauser)
{
    AActor* Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !TargetActor)
    {
        return false;
    }

    const FTransform MuzzleTransform = GetMuzzleTransform();
    MulticastPlayAttackFX(TargetActor, MuzzleTransform.GetLocation(), MuzzleTransform.Rotator());

    if (ProjectileClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = Owner;
        SpawnParams.Instigator = Cast<APawn>(Owner);
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ARTSProjectile* Projectile = Owner->GetWorld()->SpawnActor<ARTSProjectile>(
            ProjectileClass,
            MuzzleTransform,
            SpawnParams
        );

        if (Projectile)
        {
            Projectile->InitializeProjectile(
                TargetActor,
                Damage,
                DamageCauser,
                ProjectileSpeed,
                ImpactEffect,
                ImpactSound
            );
            return true;
        }
    }

    if (URTSHealthComponent* HealthComponent = TargetActor->FindComponentByClass<URTSHealthComponent>())
    {
        const bool bAppliedDamage = HealthComponent->ApplyDamage(Damage, DamageCauser);
        if (bAppliedDamage)
        {
            MulticastPlayImpactFX(TargetActor->GetActorLocation());
        }
        return bAppliedDamage;
    }

    return false;
}

void URTSCombatEffectsComponent::HandleOwnerDeath(URTSHealthComponent* HealthComponent, AActor* DamageCauser)
{
    if (AActor* Owner = GetOwner())
    {
        if (Owner->HasAuthority())
        {
            MulticastPlayDeathFX(Owner->GetActorLocation());
        }
    }
}

FTransform URTSCombatEffectsComponent::GetMuzzleTransform() const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return FTransform::Identity;
    }

    if (const USkeletalMeshComponent* SkeletalMesh = Owner->FindComponentByClass<USkeletalMeshComponent>())
    {
        if (!MuzzleSocketName.IsNone() && SkeletalMesh->DoesSocketExist(MuzzleSocketName))
        {
            return SkeletalMesh->GetSocketTransform(MuzzleSocketName);
        }
    }

    const FVector Location = Owner->GetActorLocation() + Owner->GetActorForwardVector() * MuzzleForwardOffset;
    return FTransform(Owner->GetActorRotation(), Location);
}

void URTSCombatEffectsComponent::PlayAttackAnimation() const
{
    if (!AttackMontage)
    {
        return;
    }

    USkeletalMeshComponent* SkeletalMesh = GetOwner()
        ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>()
        : nullptr;

    UAnimInstance* AnimInstance = SkeletalMesh ? SkeletalMesh->GetAnimInstance() : nullptr;
    if (AnimInstance)
    {
        AnimInstance->Montage_Play(AttackMontage);
    }
}

void URTSCombatEffectsComponent::MulticastPlayAttackFX_Implementation(
    AActor* TargetActor,
    FVector MuzzleLocation,
    FRotator MuzzleRotation
)
{
    PlayAttackAnimation();

    if (MuzzleEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleEffect, MuzzleLocation, MuzzleRotation);
    }

    if (AttackSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), AttackSound, MuzzleLocation);
    }
}

void URTSCombatEffectsComponent::MulticastPlayImpactFX_Implementation(FVector ImpactLocation)
{
    if (ImpactEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffect, ImpactLocation);
    }

    if (ImpactSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), ImpactSound, ImpactLocation);
    }
}

void URTSCombatEffectsComponent::MulticastPlayDeathFX_Implementation(FVector DeathLocation)
{
    if (DeathEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeathEffect, DeathLocation);
    }

    if (DeathSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeathSound, DeathLocation);
    }
}
