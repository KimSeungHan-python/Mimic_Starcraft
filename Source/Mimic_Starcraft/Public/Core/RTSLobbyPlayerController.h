#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Core/RTSPlayerState.h"
#include "RTSLobbyPlayerController.generated.h"

UCLASS()
class MIMIC_STARCRAFT_API ARTSLobbyPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ARTSLobbyPlayerController();

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    FName LobbyPlayerId = NAME_None;

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void RequestLeaveLobby();

    UFUNCTION(Server, Reliable)
    void ServerSubmitLobbyPlayer(FName PlayerId, const FText& PlayerName, int32 TeamNumber, ERTSRace Race, FLinearColor PlayerColor);

    UFUNCTION(Server, Reliable)
    void ServerSetLobbyPlayerTeam(FName TargetPlayerId, int32 TeamNumber);

    UFUNCTION(Server, Reliable)
    void ServerSetLobbyPlayerRace(FName TargetPlayerId, ERTSRace Race);

    UFUNCTION(Server, Reliable)
    void ServerSetLobbyPlayerColor(FName TargetPlayerId, FLinearColor PlayerColor);

    UFUNCTION(Server, Reliable)
    void ServerStartLobbyGame();

    UFUNCTION(Server, Reliable)
    void ServerLeaveLobby();

    UFUNCTION(Client, Reliable)
    void ClientRequestLobbyRegistration();

    UFUNCTION(Client, Reliable)
    void ClientReturnToLobbyBrowser();

protected:
    virtual void BeginPlay() override;
};
