#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSSelectionPanelWidget.generated.h"

class AActor;
class ARTSPlayerController;
class UTexture2D;
class URTSHealthComponent;

UENUM(BlueprintType)
enum class ERTSSelectionHealthLevel : uint8
{
    None,
    High,
    Medium,
    Low
};

USTRUCT(BlueprintType)
struct MIMIC_STARCRAFT_API FRTSSelectionPanelItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
    TObjectPtr<AActor> Actor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
    float HealthFraction = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
    ERTSSelectionHealthLevel HealthLevel = ERTSSelectionHealthLevel::None;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
    FLinearColor HealthTint = FLinearColor::White;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
    bool bIsUnit = false;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
    bool bIsBuilding = false;
};

UCLASS(Abstract, Blueprintable)
class MIMIC_STARCRAFT_API URTSSelectionPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
    TObjectPtr<ARTSPlayerController> RTSPlayerController = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
    TArray<FRTSSelectionPanelItem> SelectionItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection|Health")
    float HighHealthThreshold = 0.66f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection|Health")
    float MediumHealthThreshold = 0.33f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection|Health")
    FLinearColor HighHealthTint = FLinearColor(0.1f, 0.85f, 0.2f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection|Health")
    FLinearColor MediumHealthTint = FLinearColor(1.0f, 0.8f, 0.05f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection|Health")
    FLinearColor LowHealthTint = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);

    UFUNCTION(BlueprintCallable, Category = "RTS Selection")
    void BindToPlayerController(ARTSPlayerController* InPlayerController);

    UFUNCTION(BlueprintCallable, Category = "RTS Selection")
    void RefreshSelectionItems();

    UFUNCTION(BlueprintPure, Category = "RTS Selection")
    int32 GetSelectionItemCount() const { return SelectionItems.Num(); }

    UFUNCTION(BlueprintPure, Category = "RTS Selection")
    bool GetSelectionItemAtIndex(int32 Index, FRTSSelectionPanelItem& OutItem) const;

    UFUNCTION(BlueprintPure, Category = "RTS Selection|Health")
    ERTSSelectionHealthLevel GetHealthLevelForFraction(float HealthFraction) const;

    UFUNCTION(BlueprintPure, Category = "RTS Selection|Health")
    FLinearColor GetHealthTintForLevel(ERTSSelectionHealthLevel HealthLevel) const;

    UFUNCTION(BlueprintImplementableEvent, Category = "RTS Selection")
    void OnSelectionPanelUpdated(const TArray<FRTSSelectionPanelItem>& UpdatedItems);

protected:
    UFUNCTION()
    void HandleBoundHealthChanged(URTSHealthComponent* HealthComponent, float CurrentHealth, float MaxHealth);

private:
    UPROPERTY()
    TArray<TObjectPtr<URTSHealthComponent>> BoundHealthComponents;

    void ClearHealthBindings();
    void BindHealthComponent(URTSHealthComponent* HealthComponent);
    FRTSSelectionPanelItem BuildSelectionItem(AActor* Actor) const;
};
