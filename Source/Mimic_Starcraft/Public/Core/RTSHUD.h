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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Health Bars")
    bool bDrawSelectedActorHealthBars = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Health Bars")
    float HealthBarWidth = 54.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Health Bars")
    float HealthBarHeight = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Health Bars")
    float HealthBarWorldOffsetZ = 24.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Health Bars")
    FLinearColor HealthBarBackgroundColor = FLinearColor(0.02f, 0.02f, 0.02f, 0.85f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Health Bars")
    FLinearColor HighHealthColor = FLinearColor(0.1f, 0.85f, 0.2f, 0.95f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Health Bars")
    FLinearColor MediumHealthColor = FLinearColor(1.0f, 0.78f, 0.05f, 0.95f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Health Bars")
    FLinearColor LowHealthColor = FLinearColor(1.0f, 0.1f, 0.05f, 0.95f);

protected:
    void DrawSelectionBox(const class ARTSPlayerController* RTSPC);
    void DrawSelectedActorHealthBars(const class ARTSPlayerController* RTSPC);
    void DrawActorHealthBar(AActor* Actor, const class ARTSPlayerController* RTSPC);
    FLinearColor GetHealthBarColor(float HealthFraction) const;
};
