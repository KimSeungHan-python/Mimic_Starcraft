#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "RTSPlayerState.h"
#include "RTSGameInstance.generated.h"

class ARTSLobbyPlayerController;
class UUserWidget;

USTRUCT(BlueprintType)
struct MIMIC_STARCRAFT_API FRTSLobbyPlayerInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    FName PlayerId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    FText PlayerName;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    int32 TeamNumber = 1;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    ERTSRace Race = ERTSRace::Terran;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    FLinearColor PlayerColor = FLinearColor::Blue;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    bool bIsRoomHost = false;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    bool bIsLocalPlayer = false;
};

USTRUCT(BlueprintType)
struct MIMIC_STARCRAFT_API FRTSRoomInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    FName RoomId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    FText RoomName;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    FText HostName;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    FName HostPlayerId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    FName MapName = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    int32 CurrentPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    int32 MaxPlayers = 2;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    bool bIsJoinable = true;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    TArray<FRTSLobbyPlayerInfo> Players;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRTSRoomListChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTSActiveRoomChanged, const FRTSRoomInfo&, UpdatedRoom);

UCLASS()
class MIMIC_STARCRAFT_API URTSGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    URTSGameInstance();

    virtual void Init() override;
    virtual void Shutdown() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|Maps")
    FName MainMenuMapName = TEXT("MainMenu");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|Maps")
    FName LobbyBrowserMapName = TEXT("LobbyBrowser");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|Maps")
    FName LobbyMapName = TEXT("Lobby");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|Maps")
    FName GameMapName = TEXT("GPTTestMap");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|Maps")
    TArray<FName> AvailableGameMapNames;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|Rooms")
    int32 DefaultMaxPlayers = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Lobby|Teams")
    int32 MinLobbyTeamNumber = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Lobby|Teams")
    int32 MaxLobbyTeamNumber = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Lobby|Colors")
    TArray<FLinearColor> AvailablePlayerColors;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|UI")
    TSubclassOf<UUserWidget> MainMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|UI")
    TSubclassOf<UUserWidget> LobbyBrowserWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|UI")
    TSubclassOf<UUserWidget> LobbyWidgetClass;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Flow|UI")
    TObjectPtr<UUserWidget> ActiveFlowWidget;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Flow|Rooms")
    TArray<FRTSRoomInfo> CachedRooms;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Flow|Rooms")
    FRTSRoomInfo ActiveRoom;

    UPROPERTY(BlueprintAssignable, Category = "RTS Flow|Rooms")
    FRTSRoomListChanged OnRoomListChanged;

    UPROPERTY(BlueprintAssignable, Category = "RTS Flow|Rooms")
    FRTSActiveRoomChanged OnActiveRoomChanged;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    bool bLocalPlayerIsRoomHost = false;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    FName LocalLobbyPlayerId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    FText LocalLobbyPlayerName;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    int32 SelectedTeamNumber = 1;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    ERTSRace SelectedRace = ERTSRace::Terran;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    FLinearColor SelectedPlayerColor = FLinearColor::Blue;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    FName SelectedGameMapName = NAME_None;

    UFUNCTION(BlueprintCallable, Category = "RTS Flow")
    void OpenMainMenu();

    UFUNCTION(BlueprintCallable, Category = "RTS Flow")
    void OpenLobbyBrowser();

    UFUNCTION(BlueprintCallable, Category = "RTS Flow")
    void OpenLobby();

    UFUNCTION(BlueprintCallable, Category = "RTS Flow")
    void StartGameFromLobby();

    UFUNCTION(BlueprintCallable, Category = "RTS Flow|Rooms")
    void RefreshLocalRoomList();

    UFUNCTION(BlueprintCallable, Category = "RTS Flow|Rooms")
    bool HostLocalRoom(const FText& RoomName, const FText& HostName);

    UFUNCTION(BlueprintCallable, Category = "RTS Flow|Rooms")
    bool HostLocalRoomOnMap(const FText& RoomName, const FText& HostName, FName MapName);

    UFUNCTION(BlueprintCallable, Category = "RTS Flow|Rooms")
    bool JoinLocalRoom(FName RoomId);

    UFUNCTION(BlueprintCallable, Category = "RTS Flow|Rooms")
    bool LeaveActiveRoom();

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    bool IsLocalPlayerRoomHost() const;

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    bool CanLocalPlayerEditLobbyPlayer(FName PlayerId) const;

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    FName GetLocalLobbyPlayerId() const;

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    FName EnsureLocalLobbyPlayerId();

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void SetLocalLobbyPlayerName(const FText& InPlayerName);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool SetSelectedTeamNumber(int32 InTeamNumber);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool SetLobbyPlayerTeam(FName PlayerId, int32 InTeamNumber);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool SetLobbyPlayerTeamByIndex(int32 PlayerIndex, int32 InTeamNumber);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void SetSelectedRace(ERTSRace InRace);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool SetLobbyPlayerRace(FName PlayerId, ERTSRace InRace);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void SetSelectedPlayerColor(FLinearColor InColor);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool TrySetSelectedPlayerColor(FLinearColor InColor);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool SetLobbyPlayerColor(FName PlayerId, FLinearColor InColor);

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    bool IsPlayerColorAvailable(FLinearColor InColor, FName IgnoredPlayerId) const;

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    FLinearColor GetFirstAvailablePlayerColor(FName IgnoredPlayerId) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void SetSelectedGameMapName(FName InMapName);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool SetActiveRoomMap(FName InMapName);

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    bool GetLocalLobbyPlayerInfo(FRTSLobbyPlayerInfo& OutPlayerInfo) const;

    UFUNCTION(BlueprintPure, Category = "RTS Lobby")
    bool GetLobbyPlayerInfoByIndex(int32 PlayerIndex, FRTSLobbyPlayerInfo& OutPlayerInfo) const;

    UFUNCTION(BlueprintCallable, Category = "RTS Flow|UI")
    void ShowFlowWidgetForCurrentMap();

    void ApplyNetworkRoomState(const FRTSRoomInfo& InRoomInfo);
    void UpdateAdvertisedRoomSession(const FRTSRoomInfo& InRoomInfo);

protected:
    void OpenConfiguredLevel(FName MapName) const;

private:
    void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
    void BroadcastRoomStateChanged();
    void NormalizeActiveRoom();
    void RemoveActiveFlowWidget();
    void ShowFlowWidgetForCurrentMap(UWorld* World);
    void ShowFlowWidget(TSubclassOf<UUserWidget> WidgetClass, UWorld* World);

    bool CreateOnlineRoomSession();
    void OpenLobbyAsListenServer();
    void RefreshFallbackRoomList();
    bool JoinFallbackRoom(FName RoomId);
    bool LeaveFallbackRoom();
    bool IsOnlineSessionAvailable() const;
    IOnlineSessionPtr GetOnlineSessionInterface() const;
    ARTSLobbyPlayerController* GetLobbyPlayerController() const;
    FRTSRoomInfo MakeRoomInfoFromSessionResult(const FOnlineSessionSearchResult& SearchResult) const;
    const FOnlineSessionSearchResult* FindSessionResultByRoomId(FName RoomId) const;
    void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

    FName GetOrCreateLocalLobbyPlayerId();
    FName ResolveGameMapName(FName RequestedMapName) const;
    FText MakeDefaultPlayerName() const;
    FRTSLobbyPlayerInfo MakeLocalLobbyPlayerInfo(const FText& PlayerName, bool bRoomHost);
    int32 FindLobbyPlayerIndex(FName PlayerId) const;
    int32 FindLocalLobbyPlayerIndex() const;
    int32 ClampLobbyTeamNumber(int32 InTeamNumber) const;
    int32 GetBalancedTeamNumber() const;
    bool CanEditLobbyPlayer(FName PlayerId) const;
    bool AreLobbyColorsEqual(const FLinearColor& A, const FLinearColor& B) const;
    void SyncSelectedValuesFromLocalPlayer();

    TSharedPtr<FOnlineSessionSearch> RoomSessionSearch;
    FDelegateHandle CreateSessionCompleteDelegateHandle;
    FDelegateHandle FindSessionsCompleteDelegateHandle;
    FDelegateHandle JoinSessionCompleteDelegateHandle;
    FDelegateHandle DestroySessionCompleteDelegateHandle;
    bool bCreateSessionAfterDestroy = false;
};
