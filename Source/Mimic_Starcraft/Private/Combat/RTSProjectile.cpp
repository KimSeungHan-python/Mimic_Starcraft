#include "Combat/RTSProjectile.h"

#include "Components/RTSHealthComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ARTSProjectile::ARTSProjectile()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
}

void ARTSProjectile::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!HasAuthority())
    {
        return;
    }

    if (!TargetActor)
    {
        Destroy();
        return;
    }

    const FVector CurrentLocation = GetActorLocation();
    const FVector TargetLocation = TargetActor->GetActorLocation();
    FVector Direction = TargetLocation - CurrentLocation;

    const float Distance = Direction.Size();
    if (Distance <= ImpactRadius)
    {
        ImpactTarget();
        return;
    }

    if (Distance <= KINDA_SMALL_NUMBER)
    {
        ImpactTarget();
        return;
    }

    Direction /= Distance;

    const float Step = FMath::Min(Distance, Speed * DeltaTime);
    SetActorLocation(CurrentLocation + Direction * Step, true);
    SetActorRotation(Direction.Rotation());
}

void ARTSProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARTSProjectile, TargetActor);
    DOREPLIFETIME(ARTSProjectile, DamageCauser);
    DOREPLIFETIME(ARTSProjectile, Damage);
}

void ARTSProjectile::InitializeProjectile(
    AActor* InTargetActor,
    float InDamage,
    AActor* InDamageCauser,
    float InSpeed,
    UParticleSystem* InImpactEffect,
    USoundBase* InImpactSound
)
{
    if (!HasAuthority())
    {
        return;
    }

    TargetActor = InTargetActor;
    Damage = FMath::Max(0.0f, InDamage);
    DamageCauser = InDamageCauser;
    Speed = FMath::Max(1.0f, InSpeed);
    ImpactEffect = InImpactEffect;
    ImpactSound = InImpactSound;

    SetLifeSpan(FMath::Max(0.1f, MaxLifetime));
}

void ARTSProjectile::ImpactTarget()
{
    if (!HasAuthority())
    {
        return;
    }

    const FVector ImpactLocation = TargetActor ? TargetActor->GetActorLocation() : GetActorLocation();

    if (URTSHealthComponent* HealthComponent = TargetActor ? TargetActor->FindComponentByClass<URTSHealthComponent>() : nullptr)
    {
        HealthComponent->ApplyDamage(Damage, DamageCauser ? DamageCauser.Get() : GetOwner());
    }

    MulticastPlayImpactFX(ImpactLocation, ImpactEffect, ImpactSound);
    Destroy();
}

void ARTSProjectile::MulticastPlayImpactFX_Implementation(
    FVector ImpactLocation,
    UParticleSystem* EffectTemplate,
    USoundBase* SoundTemplate
)
{
    if (EffectTemplate)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EffectTemplate, ImpactLocation);
    }

    if (SoundTemplate)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundTemplate, ImpactLocation);
    }
}
