#include "Core/RTSCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

ARTSCameraPawn::ARTSCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComponent"));
    SpringArmComponent->SetupAttachment(SceneRoot);
    SpringArmComponent->TargetArmLength = 1400.0f;
    SpringArmComponent->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));
    SpringArmComponent->bDoCollisionTest = false;

    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(SpringArmComponent);
}

void ARTSCameraPawn::PawnClientRestart()
{
    Super::PawnClientRestart();

    AddCameraMappingContext();
}

void ARTSCameraPawn::AddCameraMappingContext()
{
    if (!CameraMappingContext)
    {
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController)
    {
        return;
    }

    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
    if (!LocalPlayer)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

    if (!Subsystem)
    {
        return;
    }

    Subsystem->AddMappingContext(CameraMappingContext, CameraMappingPriority);
}

void ARTSCameraPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    ApplyCameraMovement(DeltaTime);
    ApplyCameraZoom(DeltaTime);
}

void ARTSCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);

    if (!EnhancedInputComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("RTSCameraPawn: EnhancedInputComponent is null."));
        return;
    }

    if (CameraZoomAction)
    {
        EnhancedInputComponent->BindAction(
            CameraZoomAction,
            ETriggerEvent::Triggered,
            this,
            &ARTSCameraPawn::HandleCameraZoom
        );

        EnhancedInputComponent->BindAction(
            CameraZoomAction,
            ETriggerEvent::Completed,
            this,
            &ARTSCameraPawn::HandleCameraZoomCompleted
        );

        EnhancedInputComponent->BindAction(
            CameraZoomAction,
            ETriggerEvent::Canceled,
            this,
            &ARTSCameraPawn::HandleCameraZoomCompleted
        );
    }
}

void ARTSCameraPawn::HandleCameraZoom(const FInputActionValue& Value)
{
    PendingZoomInput = Value.Get<float>();
}

void ARTSCameraPawn::HandleCameraZoomCompleted(const FInputActionValue& Value)
{
    PendingZoomInput = 0.0f;
}

FVector2D ARTSCameraPawn::GetEdgeScrollInput() const
{
    if (!bEnableEdgeScroll)
    {
        return FVector2D::ZeroVector;
    }

    const APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController)
    {
        return FVector2D::ZeroVector;
    }

    int32 ViewportX = 0;
    int32 ViewportY = 0;
    PlayerController->GetViewportSize(ViewportX, ViewportY);

    if (ViewportX <= 0 || ViewportY <= 0)
    {
        return FVector2D::ZeroVector;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;

    if (!PlayerController->GetMousePosition(MouseX, MouseY))
    {
        return FVector2D::ZeroVector;
    }

    FVector2D EdgeInput = FVector2D::ZeroVector;

    if (MouseX <= EdgeScrollPadding)
    {
        EdgeInput.X -= 1.0f;
    }
    else if (MouseX >= ViewportX - EdgeScrollPadding)
    {
        EdgeInput.X += 1.0f;
    }

    if (MouseY <= EdgeScrollPadding)
    {
        EdgeInput.Y += 1.0f;
    }
    else if (MouseY >= ViewportY - EdgeScrollPadding)
    {
        EdgeInput.Y -= 1.0f;
    }

    return EdgeInput;
}

void ARTSCameraPawn::ApplyCameraMovement(float DeltaTime)
{
    const FVector2D EdgeInput = GetEdgeScrollInput();

    const float RightInput = EdgeInput.X;
    const float ForwardInput = EdgeInput.Y;

    if (FMath::IsNearlyZero(RightInput) && FMath::IsNearlyZero(ForwardInput))
    {
        return;
    }

    FVector Forward = GetActorForwardVector();
    Forward.Z = 0.0f;
    Forward.Normalize();

    FVector Right = GetActorRightVector();
    Right.Z = 0.0f;
    Right.Normalize();

    const FVector MoveDelta =
        (Forward * ForwardInput + Right * RightInput) * MoveSpeed * DeltaTime;

    AddActorWorldOffset(MoveDelta, true);
}

void ARTSCameraPawn::ApplyCameraZoom(float DeltaTime)
{
    if (!SpringArmComponent || FMath::IsNearlyZero(PendingZoomInput))
    {
        return;
    }

    SpringArmComponent->TargetArmLength = FMath::Clamp(
        SpringArmComponent->TargetArmLength - PendingZoomInput * ZoomSpeed * DeltaTime,
        MinZoomDistance,
        MaxZoomDistance
    );

    PendingZoomInput = 0.0f;
}
