#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/RTSGameInstance.h"
#include "RTSLobbyPlayerSlotWidget.generated.h"

class URTSLobbyWidget;

UCLASS(Abstract, Blueprintable)
class MIMIC_STARCRAFT_API URTSLobbyPlayerSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    FRTSLobbyPlayerInfo PlayerInfo;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    TObjectPtr<URTSLobbyWidget> OwnerLobbyWidget;

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void SetupPlayerSlot(const FRTSLobbyPlayerInfo& InPlayerInfo, URTSLobbyWidget* InOwnerLobbyWidget);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void UpdatePlayerInfo(const FRTSLobbyPlayerInfo& InPlayerInfo);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void SetOwnerLobbyWidget(URTSLobbyWidget* InOwnerLobbyWidget);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool TrySetTeam(int32 InTeamNumber);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool TrySetRace(ERTSRace InRace);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool TrySetPlayerColor(FLinearColor InColor);

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    bool IsLocalPlayerSlot() const;

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    bool IsRoomHostSlot() const;

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    bool CanLocalPlayerEditSlot() const;

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    bool CanLocalPlayerChangeTeam() const;

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    bool CanLocalPlayerChangeRace() const;

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    bool CanLocalPlayerChangeColor() const;

    UFUNCTION(BlueprintImplementableEvent, Category = "RTS Lobby")
    void OnPlayerInfoUpdated(const FRTSLobbyPlayerInfo& InPlayerInfo);

    UFUNCTION(BlueprintImplementableEvent, Category = "RTS Lobby")
    void OnPlayerActionRejected(const FRTSLobbyPlayerInfo& InPlayerInfo);
};
