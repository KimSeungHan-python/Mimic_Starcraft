#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RTSHUD.generated.h"

UCLASS()
class MIMIC_STARCRAFT_API ARTSHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection")
    FLinearColor SelectionFillColor = FLinearColor(0.1f, 0.55f, 1.0f, 0.12f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection")
    FLinearColor SelectionBorderColor = FLinearColor(0.25f, 0.75f, 1.0f, 0.9f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection")
    float SelectionBorderThickness = 1.5f;
};
