#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/RTSGameInstance.h"
#include "RTSLobbyWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class MIMIC_STARCRAFT_API URTSLobbyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    TObjectPtr<URTSGameInstance> RTSGameInstance;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    FRTSRoomInfo ActiveRoom;

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void BindToGameInstance(URTSGameInstance* InGameInstance);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void RefreshActiveRoom();

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void StartGame();

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void ReturnToRoomBrowser();

    UFUNCTION(BlueprintImplementableEvent, Category = "RTS Lobby")
    void OnActiveRoomUpdated(const FRTSRoomInfo& UpdatedRoom);
};
