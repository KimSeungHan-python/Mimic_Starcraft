#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTSCameraPawn.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class MIMIC_STARCRAFT_API ARTSCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    ARTSCameraPawn();

    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void PawnClientRestart() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USpringArmComponent> SpringArmComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UCameraComponent> CameraComponent;


protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
    TObjectPtr<UInputMappingContext> CameraMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
    TObjectPtr<UInputAction> CameraZoomAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
    int32 CameraMappingPriority = 0;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Camera")
    float MoveSpeed = 1800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Camera")
    float ZoomSpeed = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Camera")
    float MinZoomDistance = 700.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Camera")
    float MaxZoomDistance = 2600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Camera")
    bool bEnableEdgeScroll = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Camera", meta = (EditCondition = "bEnableEdgeScroll"))
    float EdgeScrollPadding = 24.0f;

private:
    float PendingZoomInput = 0.0f;

private:
    void AddCameraMappingContext();

    void HandleCameraZoom(const FInputActionValue& Value);
    void HandleCameraZoomCompleted(const FInputActionValue& Value);

    FVector2D GetEdgeScrollInput() const;
    void ApplyCameraMovement(float DeltaTime);
    void ApplyCameraZoom(float DeltaTime);

};
