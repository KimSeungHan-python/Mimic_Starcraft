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
    virtual void NativeDestruct() override;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    TObjectPtr<URTSGameInstance> RTSGameInstance;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    FRTSRoomInfo ActiveRoom;

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void BindToGameInstance(URTSGameInstance* InGameInstance);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void RefreshActiveRoom();

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool StartGame();

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    bool IsLocalPlayerRoomHost() const;

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    FName GetLocalLobbyPlayerId() const;

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool SetSelectedTeamNumber(int32 InTeamNumber);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool SetPlayerTeam(FName PlayerId, int32 InTeamNumber);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool SetPlayerTeamByIndex(int32 PlayerIndex, int32 InTeamNumber);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void SetSelectedRace(ERTSRace InRace);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool SetPlayerRace(FName PlayerId, ERTSRace InRace);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void SetSelectedPlayerColor(FLinearColor InColor);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool TrySetSelectedPlayerColor(FLinearColor InColor);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool SetPlayerColor(FName PlayerId, FLinearColor InColor);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool LeaveRoom();

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void ReturnToRoomBrowser();

    UFUNCTION(BlueprintImplementableEvent, Category = "RTS Lobby")
    void OnActiveRoomUpdated(const FRTSRoomInfo& UpdatedRoom);

protected:
    UFUNCTION()
    void HandleActiveRoomChanged(const FRTSRoomInfo& UpdatedRoom);
};
