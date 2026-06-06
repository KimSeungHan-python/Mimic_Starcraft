#include "Core/RTSHUD.h"

#include "Core/RTSPlayerController.h"

void ARTSHUD::DrawHUD()
{
    Super::DrawHUD();

    const ARTSPlayerController* RTSPC = Cast<ARTSPlayerController>(PlayerOwner);
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
