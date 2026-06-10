#include "Core/RTSHUD.h"

#include "Components/RTSHealthComponent.h"
#include "Core/RTSPlayerController.h"

void ARTSHUD::DrawHUD()
{
    Super::DrawHUD();

    const ARTSPlayerController* RTSPC = Cast<ARTSPlayerController>(PlayerOwner);
    if (!RTSPC)
    {
        return;
    }

    DrawSelectedActorHealthBars(RTSPC);
    DrawSelectionBox(RTSPC);
}

void ARTSHUD::DrawSelectionBox(const ARTSPlayerController* RTSPC)
{
    if (!RTSPC || !RTSPC->IsSelectionDragging())
    {
        return;
    }

    const FVector2D Start = RTSPC->GetSelectionDragStart();
    const FVector2D End = RTSPC->GetSelectionDragEnd();

    const float MinX = FMath::Min(Start.X, End.X);
    const float MaxX = FMath::Max(Start.X, End.X);
    const float MinY = FMath::Min(Start.Y, End.Y);
    const float MaxY = FMath::Max(Start.Y, End.Y);

    const float Width = MaxX - MinX;
    const float Height = MaxY - MinY;

    if (Width <= 1.0f || Height <= 1.0f)
    {
        return;
    }

    DrawRect(SelectionFillColor, MinX, MinY, Width, Height);

    DrawLine(MinX, MinY, MaxX, MinY, SelectionBorderColor, SelectionBorderThickness);
    DrawLine(MaxX, MinY, MaxX, MaxY, SelectionBorderColor, SelectionBorderThickness);
    DrawLine(MaxX, MaxY, MinX, MaxY, SelectionBorderColor, SelectionBorderThickness);
    DrawLine(MinX, MaxY, MinX, MinY, SelectionBorderColor, SelectionBorderThickness);
}

void ARTSHUD::DrawSelectedActorHealthBars(const ARTSPlayerController* RTSPC)
{
    if (!bDrawSelectedActorHealthBars || !RTSPC)
    {
        return;
    }

    for (AActor* Actor : RTSPC->GetSelectedActors())
    {
        DrawActorHealthBar(Actor, RTSPC);
    }
}

void ARTSHUD::DrawActorHealthBar(AActor* Actor, const ARTSPlayerController* RTSPC)
{
    if (!Actor || !RTSPC || !Canvas || !RTSPC->IsActorVisibleToLocalPlayer(Actor))
    {
        return;
    }

    const URTSHealthComponent* HealthComponent = Actor->FindComponentByClass<URTSHealthComponent>();
    if (!HealthComponent || !HealthComponent->IsAlive())
    {
        return;
    }

    FVector Origin;
    FVector Extent;
    Actor->GetActorBounds(false, Origin, Extent);

    const FVector BarWorldLocation = Origin + FVector(0.0f, 0.0f, Extent.Z + HealthBarWorldOffsetZ);

    FVector2D ScreenPosition;
    if (!RTSPC->ProjectWorldLocationToScreen(BarWorldLocation, ScreenPosition))
    {
        return;
    }

    const float HealthFraction = HealthComponent->GetHealthFraction();
    const float BarX = ScreenPosition.X - HealthBarWidth * 0.5f;
    const float BarY = ScreenPosition.Y - HealthBarHeight * 0.5f;

    DrawRect(
        HealthBarBackgroundColor,
        BarX - 1.0f,
        BarY - 1.0f,
        HealthBarWidth + 2.0f,
        HealthBarHeight + 2.0f
    );

    DrawRect(
        FLinearColor(0.08f, 0.08f, 0.08f, 0.85f),
        BarX,
        BarY,
        HealthBarWidth,
        HealthBarHeight
    );

    DrawRect(
        GetHealthBarColor(HealthFraction),
        BarX,
        BarY,
        HealthBarWidth * HealthFraction,
        HealthBarHeight
    );
}

FLinearColor ARTSHUD::GetHealthBarColor(float HealthFraction) const
{
    if (HealthFraction >= 0.66f)
    {
        return HighHealthColor;
    }

    if (HealthFraction >= 0.33f)
    {
        return MediumHealthColor;
    }

    return LowHealthColor;
}
