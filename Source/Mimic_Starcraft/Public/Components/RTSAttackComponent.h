#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTSAttackComponent.generated.h"

class URTSHealthComponent;

UENUM(BlueprintType)
enum class ERTSAttackCommandMode : uint8
{
    Idle,
    AttackTarget,
    AttackMove
};

UCLASS(ClassGroup = (RTS), meta = (BlueprintSpawnableComponent))
class MIMIC_STARCRAFT_API URTSAttackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URTSAttackComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Attack")
    bool bCanAttack = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Attack")
    float AttackDamage = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Attack")
    float AttackRange = 175.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Attack")
    float AttackCooldown = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Attack")
    float AttackMoveAcceptanceRadius = 65.0f;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Attack")
    ERTSAttackCommandMode AttackMode = ERTSAttackCommandMode::Idle;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Attack")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "RTS Attack")
    FVector AttackMoveLocation = FVector::ZeroVector;

    UFUNCTION(BlueprintCallable, Category = "RTS Attack")
    void ConfigureAttackStats(bool bNewCanAttack, float NewDamage, float NewRange, float NewCooldown);

    UFUNCTION(BlueprintCallable, Category = "RTS Attack")
    bool IssueAttackTarget(AActor* NewTargetActor);

    UFUNCTION(BlueprintCallable, Category = "RTS Attack")
    bool IssueAttackMove(const FVector& TargetLocation);

    UFUNCTION(BlueprintCallable, Category = "RTS Attack")
    void StopAttackCommand();

    UFUNCTION(BlueprintPure, Category = "RTS Attack")
    bool HasActiveAttackCommand() const { return AttackMode != ERTSAttackCommandMode::Idle; }

    UFUNCTION(BlueprintPure, Category = "RTS Attack")
    bool CanAttackTarget(AActor* Candidate) const;

protected:
    float CooldownRemaining = 0.0f;

    int32 GetOwnerTeamNumber() const;
    int32 GetActorTeamNumber(AActor* Actor) const;
    URTSHealthComponent* GetTargetHealth(AActor* Actor) const;
    bool IsTargetInRange(AActor* Candidate) const;
    bool HasReachedAttackMoveLocation() const;
    AActor* FindEnemyInAttackRange() const;
    void ProcessAttackTarget(float DeltaTime);
    void ProcessAttackMove(float DeltaTime);
    void AttackTargetNow(AActor* Candidate);
    void MoveOwnerToward(const FVector& WorldLocation);
    void StopOwnerMovement();
};
