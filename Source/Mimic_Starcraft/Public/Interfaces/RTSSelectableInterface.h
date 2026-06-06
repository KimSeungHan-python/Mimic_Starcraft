#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RTSSelectableInterface.generated.h"

class ARTSPlayerController;
class ARTSPlayerState;

UINTERFACE(BlueprintType)
class MIMIC_STARCRAFT_API URTSSelectableInterface : public UInterface
{
    GENERATED_BODY()
};

class MIMIC_STARCRAFT_API IRTSSelectableInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Selection")
    bool CanBeSelectedBy(ARTSPlayerController* SelectingController) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Selection")
    void SetSelectionState(bool bSelected);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Selection")
    bool IsSelected() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Team")
    int32 GetSelectableTeamNumber() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS|Team")
    bool IsOwnedByPlayerState(ARTSPlayerState* PlayerState) const;
};
