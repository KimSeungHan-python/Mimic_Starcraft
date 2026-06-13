#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Core/RTSGameInstance.h"
#include "RTSLobbyGameMode.generated.h"

class ARTSLobbyGameState;
class ARTSLobbyPlayerController;

UCLASS()
class MIMIC_STARCRAFT_API ARTSLobbyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ARTSLobbyGameMode();

    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

    bool RegisterLobbyPlayer(
        ARTSLobbyPlayerController* PlayerController,
        FName PlayerId,
        const FText& PlayerName,
        int32 TeamNumber,
        ERTSRace Race,
        const FLinearColor& PlayerColor
    );

    bool SetLobbyPlayerTeam(ARTSLobbyPlayerController* RequestingPlayer, FName TargetPlayerId, int32 TeamNumber);
    bool SetLobbyPlayerName(ARTSLobbyPlayerController* RequestingPlayer, FName TargetPlayerId, const FText& PlayerName);
    bool SetLobbyPlayerRace(ARTSLobbyPlayerController* RequestingPlayer, FName TargetPlayerId, ERTSRace Race);
    bool SetLobbyPlayerColor(ARTSLobbyPlayerController* RequestingPlayer, FName TargetPlayerId, const FLinearColor& PlayerColor);
    bool StartLobbyGame(ARTSLobbyPlayerController* RequestingPlayer);
    bool LeaveLobby(ARTSLobbyPlayerController* RequestingPlayer);

protected:
    UPROPERTY(EditDefaultsOnly, Category = "RTS Lobby")
    int32 MinLobbyTeamNumber = 1;

    UPROPERTY(EditDefaultsOnly, Category = "RTS Lobby")
    int32 MaxLobbyTeamNumber = 2;

private:
    void InitializeRoomIfNeeded();
    void PublishRoom(const FRTSRoomInfo& InRoomInfo);
    FRTSRoomInfo GetMutableRoom() const;
    int32 FindPlayerIndex(const FRTSRoomInfo& RoomInfo, FName PlayerId) const;
    int32 ClampTeamNumber(int32 TeamNumber) const;
    int32 GetBalancedTeamNumber(const FRTSRoomInfo& RoomInfo) const;
    bool CanEditPlayer(const FRTSRoomInfo& RoomInfo, FName RequestingPlayerId, FName TargetPlayerId) const;
    bool IsRoomHost(const FRTSRoomInfo& RoomInfo, FName PlayerId) const;
    bool IsColorAvailable(const FRTSRoomInfo& RoomInfo, const FLinearColor& PlayerColor, FName IgnoredPlayerId) const;
    FLinearColor GetFirstAvailableColor(const FRTSRoomInfo& RoomInfo, FName IgnoredPlayerId) const;
    void NormalizeRoom(FRTSRoomInfo& RoomInfo) const;
    ARTSLobbyGameState* GetLobbyGameState() const;
};
