#include "Components/RTSAttackComponent.h"

#include "Buildings/RTSBuilding.h"
#include "Components/RTSCombatEffectsComponent.h"
#include "Components/RTSHealthComponent.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "Spatial/RTSActorSpatialIndex.h"
#include "Units/RTSUnitBase.h"

URTSAttackComponent::URTSAttackComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickInterval = 0.08f;
    SetIsReplicatedByDefault(true);
}

void URTSAttackComponent::BeginPlay()
{
    Super::BeginPlay();

    SetComponentTickEnabled(GetOwner() && GetOwner()->HasAuthority() && AttackMode != ERTSAttackCommandMode::Idle);
}

void URTSAttackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    CooldownRemaining = FMath::Max(0.0f, CooldownRemaining - DeltaTime);

    switch (AttackMode)
    {
    case ERTSAttackCommandMode::AttackTarget:
        ProcessAttackTarget(DeltaTime);
        break;

    case ERTSAttackCommandMode::AttackMove:
        ProcessAttackMove(DeltaTime);
        break;

    default:
        SetComponentTickEnabled(false);
        break;
    }
}

void URTSAttackComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(URTSAttackComponent, AttackMode);
    DOREPLIFETIME(URTSAttackComponent, TargetActor);
    DOREPLIFETIME(URTSAttackComponent, AttackMoveLocation);
}

void URTSAttackComponent::ConfigureAttackStats(bool bNewCanAttack, float NewDamage, float NewRange, float NewCooldown)
{
    bCanAttack = bNewCanAttack;
    AttackDamage = FMath::Max(0.0f, NewDamage);
    AttackRange = FMath::Max(1.0f, NewRange);
    AttackCooldown = FMath::Max(0.05f, NewCooldown);

    if (!bCanAttack)
    {
        StopAttackCommand();
    }
}

bool URTSAttackComponent::IssueAttackTarget(AActor* NewTargetActor)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !CanAttackTarget(NewTargetActor))
    {
        return false;
    }

    TargetActor = NewTargetActor;
    AttackMode = ERTSAttackCommandMode::AttackTarget;
    SetComponentTickEnabled(true);
    return true;
}

bool URTSAttackComponent::IssueAttackMove(const FVector& TargetLocation)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !bCanAttack)
    {
        return false;
    }

    TargetActor = nullptr;
    AttackMoveLocation = TargetLocation;
    AttackMode = ERTSAttackCommandMode::AttackMove;
    MoveOwnerToward(AttackMoveLocation);
    SetComponentTickEnabled(true);
    return true;
}

void URTSAttackComponent::StopAttackCommand()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    TargetActor = nullptr;
    AttackMode = ERTSAttackCommandMode::Idle;
    SetComponentTickEnabled(false);
}

bool URTSAttackComponent::CanAttackTarget(AActor* Candidate) const
{
    if (!bCanAttack || !Candidate || Candidate == GetOwner())
    {
        return false;
    }

    const int32 OwnerTeam = GetOwnerTeamNumber();
    const int32 TargetTeam = GetActorTeamNumber(Candidate);
    if (OwnerTeam < 0 || TargetTeam < 0 || OwnerTeam == TargetTeam)
    {
        return false;
    }

    const URTSHealthComponent* HealthComponent = GetTargetHealth(Candidate);
    return HealthComponent && HealthComponent->IsAlive();
}

int32 URTSAttackComponent::GetOwnerTeamNumber() const
{
    return GetActorTeamNumber(GetOwner());
}

int32 URTSAttackComponent::GetActorTeamNumber(AActor* Actor) const
{
    if (const ARTSUnitBase* Unit = Cast<ARTSUnitBase>(Actor))
    {
        return Unit->TeamNumber;
    }

    if (const ARTSBuilding* Building = Cast<ARTSBuilding>(Actor))
    {
        return Building->TeamNumber;
    }

    return INDEX_NONE;
}

URTSHealthComponent* URTSAttackComponent::GetTargetHealth(AActor* Actor) const
{
    return Actor ? Actor->FindComponentByClass<URTSHealthComponent>() : nullptr;
}

bool URTSAttackComponent::IsTargetInRange(AActor* Candidate) const
{
    if (!Candidate || !GetOwner())
    {
        return false;
    }

    FVector Delta = Candidate->GetActorLocation() - GetOwner()->GetActorLocation();
    Delta.Z = 0.0f;
    return Delta.SizeSquared() <= FMath::Square(AttackRange);
}

bool URTSAttackComponent::HasReachedAttackMoveLocation() const
{
    if (!GetOwner())
    {
        return true;
    }

    FVector Delta = AttackMoveLocation - GetOwner()->GetActorLocation();
    Delta.Z = 0.0f;
    return Delta.SizeSquared() <= FMath::Square(AttackMoveAcceptanceRadius);
}

AActor* URTSAttackComponent::FindEnemyInAttackRange() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    AActor* BestTarget = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    auto ConsiderTarget = [this, &BestTarget, &BestDistSq](AActor* Candidate)
    {
        if (!CanAttackTarget(Candidate) || !IsTargetInRange(Candidate))
        {
            return;
        }

        const float DistSq = FVector::DistSquared2D(GetOwner()->GetActorLocation(), Candidate->GetActorLocation());
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestTarget = Candidate;
        }
    };

    ARTSActorSpatialIndex* SpatialIndex = ResolveSpatialIndex();
    if (SpatialIndex && SpatialIndex->GetRegisteredActorCount() > 0)
    {
        TArray<AActor*> NearbyActors;
        SpatialIndex->QueryActorsInRadius(GetOwner()->GetActorLocation(), AttackRange, NearbyActors);

        for (AActor* NearbyActor : NearbyActors)
        {
            ConsiderTarget(NearbyActor);
        }

        return BestTarget;
    }

    for (TActorIterator<ARTSUnitBase> It(World); It; ++It)
    {
        ConsiderTarget(*It);
    }

    for (TActorIterator<ARTSBuilding> It(World); It; ++It)
    {
        ARTSBuilding* Building = *It;
        if (Building && !Building->bIsPreviewBuilding)
        {
            ConsiderTarget(Building);
        }
    }

    return BestTarget;
}

ARTSActorSpatialIndex* URTSAttackComponent::ResolveSpatialIndex() const
{
    UWorld* World = GetWorld();
    return World ? ARTSActorSpatialIndex::GetOrCreate(World) : nullptr;
}

void URTSAttackComponent::ProcessAttackTarget(float DeltaTime)
{
    if (!CanAttackTarget(TargetActor))
    {
        StopAttackCommand();
        return;
    }

    if (!IsTargetInRange(TargetActor))
    {
        MoveOwnerToward(TargetActor->GetActorLocation());
        return;
    }

    StopOwnerMovement();
    AttackTargetNow(TargetActor);
}

void URTSAttackComponent::ProcessAttackMove(float DeltaTime)
{
    if (!CanAttackTarget(TargetActor))
    {
        TargetActor = FindEnemyInAttackRange();
    }

    if (CanAttackTarget(TargetActor))
    {
        if (!IsTargetInRange(TargetActor))
        {
            TargetActor = nullptr;
            MoveOwnerToward(AttackMoveLocation);
            return;
        }

        StopOwnerMovement();
        AttackTargetNow(TargetActor);
        return;
    }

    if (HasReachedAttackMoveLocation())
    {
        StopOwnerMovement();
        StopAttackCommand();
        return;
    }

    MoveOwnerToward(AttackMoveLocation);
}

void URTSAttackComponent::AttackTargetNow(AActor* Candidate)
{
    if (CooldownRemaining > 0.0f)
    {
        return;
    }

    URTSHealthComponent* HealthComponent = GetTargetHealth(Candidate);
    if (!HealthComponent)
    {
        return;
    }

    OnAttackStarted.Broadcast(this, Candidate);

    if (URTSCombatEffectsComponent* CombatEffectsComponent = GetOwner()->FindComponentByClass<URTSCombatEffectsComponent>())
    {
        if (CombatEffectsComponent->ExecuteAttack(Candidate, AttackDamage, GetOwner()))
        {
            CooldownRemaining = AttackCooldown;
            OnAttackResolved.Broadcast(this, Candidate);
            return;
        }
    }

    HealthComponent->ApplyDamage(AttackDamage, GetOwner());
    CooldownRemaining = AttackCooldown;
    OnAttackResolved.Broadcast(this, Candidate);
}

void URTSAttackComponent::MoveOwnerToward(const FVector& WorldLocation)
{
    if (ARTSUnitBase* Unit = Cast<ARTSUnitBase>(GetOwner()))
    {
        Unit->IssueMoveCommand(WorldLocation);
    }
}

void URTSAttackComponent::StopOwnerMovement()
{
    if (ARTSUnitBase* Unit = Cast<ARTSUnitBase>(GetOwner()))
    {
        Unit->StopMovement();

        if (TargetActor)
        {
            FVector Direction = TargetActor->GetActorLocation() - Unit->GetActorLocation();
            Direction.Z = 0.0f;
            if (!Direction.IsNearlyZero())
            {
                Unit->SetActorRotation(Direction.Rotation());
            }
        }
    }
}
