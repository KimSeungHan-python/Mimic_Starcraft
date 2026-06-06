#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "Types/RTSCommandTypes.h"
#include "RTSCommandCardWidget.generated.h"

class ARTSPlayerController;

UCLASS(Abstract, Blueprintable)
class MIMIC_STARCRAFT_API URTSCommandCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    TObjectPtr<ARTSPlayerController> RTSPlayerController;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    TArray<FRTSCommandButton> Commands;

    UFUNCTION(BlueprintCallable, Category = "RTS Command")
    void BindToPlayerController(ARTSPlayerController* InPlayerController);

    UFUNCTION(BlueprintCallable, Category = "RTS Command")
    void RefreshCommands();

    UFUNCTION(BlueprintCallable, Category = "RTS Command")
    bool ExecuteCommandAtSlot(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "RTS Command")
    bool ExecuteCommandHotkey(FKey Hotkey);

    UFUNCTION(BlueprintImplementableEvent, Category = "RTS Command")
    void OnCommandCardUpdated(const TArray<FRTSCommandButton>& UpdatedCommands);
};
