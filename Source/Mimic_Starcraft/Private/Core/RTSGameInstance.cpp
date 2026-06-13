#include "Core/RTSGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Guid.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Core/RTSLobbyPlayerController.h"
#include "UObject/UObjectGlobals.h"

namespace RTSLobbySession
{
    static const FName SessionName(TEXT("RTSGameSession"));
    static const FName RoomIdKey(TEXT("RTS_ROOM_ID"));
    static const FName RoomNameKey(TEXT("RTS_ROOM_NAME"));
    static const FName HostNameKey(TEXT("RTS_HOST_NAME"));
    static const FName MapNameKey(TEXT("RTS_MAP_NAME"));
}

URTSGameInstance::URTSGameInstance()
{
    SelectedGameMapName = GameMapName;

    AvailableGameMapNames =
    {
        GameMapName
    };

    AvailablePlayerColors =
    {
        FLinearColor::Blue,
        FLinearColor::Red,
        FLinearColor::Green,
        FLinearColor::Yellow,
        FLinearColor(0.0f, 0.85f, 1.0f, 1.0f),
        FLinearColor(1.0f, 0.45f, 0.0f, 1.0f),
        FLinearColor(0.65f, 0.25f, 1.0f, 1.0f),
        FLinearColor(1.0f, 0.1f, 0.55f, 1.0f)
    };
}

void URTSGameInstance::Init()
{
    Super::Init();

    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this,
        &URTSGameInstance::HandlePostLoadMapWithWorld
    );
}

void URTSGameInstance::Shutdown()
{
    if (IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface())
    {
        if (CreateSessionCompleteDelegateHandle.IsValid())
        {
            SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
        }

        if (FindSessionsCompleteDelegateHandle.IsValid())
        {
            SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
        }

        if (JoinSessionCompleteDelegateHandle.IsValid())
        {
            SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
        }

        if (DestroySessionCompleteDelegateHandle.IsValid())
        {
            SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
        }
    }

    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
    RemoveActiveFlowWidget();

    Super::Shutdown();
}

void URTSGameInstance::OpenMainMenu()
{
    OpenConfiguredLevel(MainMenuMapName);
}

void URTSGameInstance::OpenLobbyBrowser()
{
    RefreshLocalRoomList();
    OpenConfiguredLevel(LobbyBrowserMapName);
}

void URTSGameInstance::OpenLobby()
{
    OpenConfiguredLevel(LobbyMapName);
}

void URTSGameInstance::StartGameFromLobby()
{
    if (ARTSLobbyPlayerController* LobbyPlayerController = GetLobbyPlayerController())
    {
        LobbyPlayerController->ServerStartLobbyGame();
        return;
    }

    if (!IsLocalPlayerRoomHost())
    {
        UE_LOG(LogTemp, Warning, TEXT("Only room host can start the game."));
        return;
    }

    OpenConfiguredLevel(ResolveGameMapName(ActiveRoom.MapName));
}

void URTSGameInstance::RefreshLocalRoomList()
{
    if (IsOnlineSessionAvailable())
    {
        CachedRooms.Reset();

        IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
        if (!SessionInterface)
        {
            OnRoomListChanged.Broadcast();
            return;
        }

        if (FindSessionsCompleteDelegateHandle.IsValid())
        {
            SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
            FindSessionsCompleteDelegateHandle.Reset();
        }

        RoomSessionSearch = MakeShared<FOnlineSessionSearch>();
        RoomSessionSearch->bIsLanQuery = true;
        RoomSessionSearch->MaxSearchResults = 100;

        FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
            FOnFindSessionsCompleteDelegate::CreateUObject(this, &URTSGameInstance::HandleFindSessionsComplete)
        );

        if (!SessionInterface->FindSessions(0, RoomSessionSearch.ToSharedRef()))
        {
            SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
            FindSessionsCompleteDelegateHandle.Reset();
            OnRoomListChanged.Broadcast();
        }

        return;
    }

    RefreshFallbackRoomList();
}

bool URTSGameInstance::HostLocalRoom(const FText& RoomName, const FText& HostName)
{
    return HostLocalRoomOnMap(RoomName, HostName, SelectedGameMapName);
}

bool URTSGameInstance::HostLocalRoomOnMap(const FText& RoomName, const FText& HostName, FName MapName)
{
    const FText EffectiveRoomName = RoomName.IsEmpty()
        ? FText::FromString(TEXT("Local Room"))
        : RoomName;

    const FText EffectiveHostName = HostName.IsEmpty()
        ? MakeDefaultPlayerName()
        : HostName;

    LocalLobbyPlayerName = EffectiveHostName;
    bLocalPlayerIsRoomHost = true;
    SelectedTeamNumber = ClampLobbyTeamNumber(SelectedTeamNumber);
    SelectedGameMapName = ResolveGameMapName(MapName);

    ActiveRoom = FRTSRoomInfo();
    ActiveRoom.RoomId = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
    ActiveRoom.RoomName = EffectiveRoomName;
    ActiveRoom.MapName = SelectedGameMapName;
    ActiveRoom.MaxPlayers = FMath::Max(1, DefaultMaxPlayers);

    ActiveRoom.Players.Add(MakeLocalLobbyPlayerInfo(EffectiveHostName, true));

    NormalizeActiveRoom();

    if (IsOnlineSessionAvailable())
    {
        return CreateOnlineRoomSession();
    }

    BroadcastRoomStateChanged();
    OpenLobby();
    return true;
}

bool URTSGameInstance::JoinLocalRoom(FName RoomId)
{
    if (const FOnlineSessionSearchResult* SearchResult = FindSessionResultByRoomId(RoomId))
    {
        IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
        if (!SessionInterface)
        {
            return false;
        }

        ActiveRoom = MakeRoomInfoFromSessionResult(*SearchResult);
        SelectedGameMapName = ActiveRoom.MapName;
        bLocalPlayerIsRoomHost = false;

        if (JoinSessionCompleteDelegateHandle.IsValid())
        {
            SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
            JoinSessionCompleteDelegateHandle.Reset();
        }

        JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
            FOnJoinSessionCompleteDelegate::CreateUObject(this, &URTSGameInstance::HandleJoinSessionComplete)
        );

        return SessionInterface->JoinSession(0, RTSLobbySession::SessionName, *SearchResult);
    }

    return JoinFallbackRoom(RoomId);
}

bool URTSGameInstance::LeaveActiveRoom()
{
    if (ARTSLobbyPlayerController* LobbyPlayerController = GetLobbyPlayerController())
    {
        LobbyPlayerController->RequestLeaveLobby();
        return true;
    }

    IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
    if (SessionInterface && SessionInterface->GetNamedSession(RTSLobbySession::SessionName))
    {
        if (DestroySessionCompleteDelegateHandle.IsValid())
        {
            SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
            DestroySessionCompleteDelegateHandle.Reset();
        }

        DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
            FOnDestroySessionCompleteDelegate::CreateUObject(this, &URTSGameInstance::HandleDestroySessionComplete)
        );

        if (SessionInterface->DestroySession(RTSLobbySession::SessionName))
        {
            return true;
        }

        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
        DestroySessionCompleteDelegateHandle.Reset();
    }

    return LeaveFallbackRoom();
}

bool URTSGameInstance::JoinFallbackRoom(FName RoomId)
{
    RefreshFallbackRoomList();

    if (RoomId.IsNone()
        || ActiveRoom.RoomId != RoomId
        || ActiveRoom.CurrentPlayers >= ActiveRoom.MaxPlayers)
    {
        return false;
    }

    if (FindLocalLobbyPlayerIndex() != INDEX_NONE)
    {
        SyncSelectedValuesFromLocalPlayer();
        OpenLobby();
        return true;
    }

    bLocalPlayerIsRoomHost = false;
    SelectedTeamNumber = GetBalancedTeamNumber();
    SelectedGameMapName = ResolveGameMapName(ActiveRoom.MapName);

    const FText PlayerName = LocalLobbyPlayerName.IsEmpty()
        ? MakeDefaultPlayerName()
        : LocalLobbyPlayerName;

    ActiveRoom.Players.Add(MakeLocalLobbyPlayerInfo(PlayerName, false));

    BroadcastRoomStateChanged();
    OpenLobby();
    return true;
}

bool URTSGameInstance::LeaveFallbackRoom()
{
    const int32 LocalPlayerIndex = FindLocalLobbyPlayerIndex();
    if (LocalPlayerIndex == INDEX_NONE)
    {
        bLocalPlayerIsRoomHost = false;
        OpenLobbyBrowser();
        return false;
    }

    ActiveRoom.Players.RemoveAt(LocalPlayerIndex);
    bLocalPlayerIsRoomHost = false;

    BroadcastRoomStateChanged();
    OpenLobbyBrowser();
    return true;
}

bool URTSGameInstance::IsLocalPlayerRoomHost() const
{
    return bLocalPlayerIsRoomHost;
}

bool URTSGameInstance::CanLocalPlayerEditLobbyPlayer(FName PlayerId) const
{
    return CanEditLobbyPlayer(PlayerId);
}

FName URTSGameInstance::GetLocalLobbyPlayerId() const
{
    return LocalLobbyPlayerId;
}

FName URTSGameInstance::EnsureLocalLobbyPlayerId()
{
    return GetOrCreateLocalLobbyPlayerId();
}

void URTSGameInstance::SetLocalLobbyPlayerName(const FText& InPlayerName)
{
    LocalLobbyPlayerName = InPlayerName.IsEmpty()
        ? MakeDefaultPlayerName()
        : InPlayerName;

    const int32 LocalPlayerIndex = FindLocalLobbyPlayerIndex();
    if (LocalPlayerIndex == INDEX_NONE)
    {
        return;
    }

    ActiveRoom.Players[LocalPlayerIndex].PlayerName = LocalLobbyPlayerName;
    BroadcastRoomStateChanged();
}

bool URTSGameInstance::SetSelectedTeamNumber(int32 InTeamNumber)
{
    SelectedTeamNumber = ClampLobbyTeamNumber(InTeamNumber);

    const int32 LocalPlayerIndex = FindLocalLobbyPlayerIndex();
    if (LocalPlayerIndex == INDEX_NONE)
    {
        return true;
    }

    return SetLobbyPlayerTeam(LocalLobbyPlayerId, SelectedTeamNumber);
}

bool URTSGameInstance::SetLobbyPlayerTeam(FName PlayerId, int32 InTeamNumber)
{
    if (ARTSLobbyPlayerController* LobbyPlayerController = GetLobbyPlayerController())
    {
        LobbyPlayerController->ServerSetLobbyPlayerTeam(PlayerId, InTeamNumber);
        return true;
    }

    if (!CanEditLobbyPlayer(PlayerId))
    {
        return false;
    }

    const int32 PlayerIndex = FindLobbyPlayerIndex(PlayerId);
    if (PlayerIndex == INDEX_NONE)
    {
        return false;
    }

    ActiveRoom.Players[PlayerIndex].TeamNumber = ClampLobbyTeamNumber(InTeamNumber);

    if (PlayerId == LocalLobbyPlayerId)
    {
        SelectedTeamNumber = ActiveRoom.Players[PlayerIndex].TeamNumber;
    }

    BroadcastRoomStateChanged();
    return true;
}

bool URTSGameInstance::SetLobbyPlayerTeamByIndex(int32 PlayerIndex, int32 InTeamNumber)
{
    if (!ActiveRoom.Players.IsValidIndex(PlayerIndex))
    {
        return false;
    }

    return SetLobbyPlayerTeam(ActiveRoom.Players[PlayerIndex].PlayerId, InTeamNumber);
}

void URTSGameInstance::SetSelectedRace(ERTSRace InRace)
{
    SelectedRace = InRace;

    if (ARTSLobbyPlayerController* LobbyPlayerController = GetLobbyPlayerController())
    {
        LobbyPlayerController->ServerSetLobbyPlayerRace(GetOrCreateLocalLobbyPlayerId(), InRace);
        return;
    }

    const int32 LocalPlayerIndex = FindLocalLobbyPlayerIndex();
    if (LocalPlayerIndex == INDEX_NONE)
    {
        return;
    }

    ActiveRoom.Players[LocalPlayerIndex].Race = InRace;
    BroadcastRoomStateChanged();
}

bool URTSGameInstance::SetLobbyPlayerRace(FName PlayerId, ERTSRace InRace)
{
    if (ARTSLobbyPlayerController* LobbyPlayerController = GetLobbyPlayerController())
    {
        LobbyPlayerController->ServerSetLobbyPlayerRace(PlayerId, InRace);
        return true;
    }

    if (!CanEditLobbyPlayer(PlayerId))
    {
        return false;
    }

    const int32 PlayerIndex = FindLobbyPlayerIndex(PlayerId);
    if (PlayerIndex == INDEX_NONE)
    {
        return false;
    }

    ActiveRoom.Players[PlayerIndex].Race = InRace;

    if (PlayerId == LocalLobbyPlayerId)
    {
        SelectedRace = InRace;
    }

    BroadcastRoomStateChanged();
    return true;
}

void URTSGameInstance::SetSelectedPlayerColor(FLinearColor InColor)
{
    TrySetSelectedPlayerColor(InColor);
}

bool URTSGameInstance::TrySetSelectedPlayerColor(FLinearColor InColor)
{
    if (ARTSLobbyPlayerController* LobbyPlayerController = GetLobbyPlayerController())
    {
        LobbyPlayerController->ServerSetLobbyPlayerColor(GetOrCreateLocalLobbyPlayerId(), InColor);
        return true;
    }

    const int32 LocalPlayerIndex = FindLocalLobbyPlayerIndex();
    if (LocalPlayerIndex == INDEX_NONE)
    {
        if (!IsPlayerColorAvailable(InColor, LocalLobbyPlayerId))
        {
            return false;
        }

        SelectedPlayerColor = InColor;
        return true;
    }

    return SetLobbyPlayerColor(LocalLobbyPlayerId, InColor);
}

bool URTSGameInstance::SetLobbyPlayerColor(FName PlayerId, FLinearColor InColor)
{
    if (ARTSLobbyPlayerController* LobbyPlayerController = GetLobbyPlayerController())
    {
        LobbyPlayerController->ServerSetLobbyPlayerColor(PlayerId, InColor);
        return true;
    }

    if (!CanEditLobbyPlayer(PlayerId) || !IsPlayerColorAvailable(InColor, PlayerId))
    {
        return false;
    }

    const int32 PlayerIndex = FindLobbyPlayerIndex(PlayerId);
    if (PlayerIndex == INDEX_NONE)
    {
        return false;
    }

    ActiveRoom.Players[PlayerIndex].PlayerColor = InColor;

    if (PlayerId == LocalLobbyPlayerId)
    {
        SelectedPlayerColor = InColor;
    }

    BroadcastRoomStateChanged();
    return true;
}

bool URTSGameInstance::IsPlayerColorAvailable(FLinearColor InColor, FName IgnoredPlayerId) const
{
    for (const FRTSLobbyPlayerInfo& Player : ActiveRoom.Players)
    {
        if (Player.PlayerId == IgnoredPlayerId)
        {
            continue;
        }

        if (AreLobbyColorsEqual(Player.PlayerColor, InColor))
        {
            return false;
        }
    }

    return true;
}

FLinearColor URTSGameInstance::GetFirstAvailablePlayerColor(FName IgnoredPlayerId) const
{
    for (const FLinearColor& CandidateColor : AvailablePlayerColors)
    {
        if (IsPlayerColorAvailable(CandidateColor, IgnoredPlayerId))
        {
            return CandidateColor;
        }
    }

    for (int32 CandidateIndex = 0; CandidateIndex < 32; ++CandidateIndex)
    {
        const uint8 Hue = static_cast<uint8>((CandidateIndex * 47) % 255);
        const FLinearColor CandidateColor = FLinearColor::MakeFromHSV8(Hue, 210, 255);
        if (IsPlayerColorAvailable(CandidateColor, IgnoredPlayerId))
        {
            return CandidateColor;
        }
    }

    return FLinearColor::White;
}

void URTSGameInstance::SetSelectedGameMapName(FName InMapName)
{
    SelectedGameMapName = ResolveGameMapName(InMapName);

    if (IsLocalPlayerRoomHost() && !ActiveRoom.RoomId.IsNone())
    {
        SetActiveRoomMap(SelectedGameMapName);
    }
}

bool URTSGameInstance::SetActiveRoomMap(FName InMapName)
{
    if (!IsLocalPlayerRoomHost() || ActiveRoom.RoomId.IsNone())
    {
        return false;
    }

    ActiveRoom.MapName = ResolveGameMapName(InMapName);
    SelectedGameMapName = ActiveRoom.MapName;

    BroadcastRoomStateChanged();
    return true;
}

bool URTSGameInstance::GetLocalLobbyPlayerInfo(FRTSLobbyPlayerInfo& OutPlayerInfo) const
{
    const int32 LocalPlayerIndex = FindLocalLobbyPlayerIndex();
    if (LocalPlayerIndex == INDEX_NONE)
    {
        return false;
    }

    OutPlayerInfo = ActiveRoom.Players[LocalPlayerIndex];
    return true;
}

bool URTSGameInstance::GetLobbyPlayerInfoByIndex(int32 PlayerIndex, FRTSLobbyPlayerInfo& OutPlayerInfo) const
{
    if (!ActiveRoom.Players.IsValidIndex(PlayerIndex))
    {
        return false;
    }

    OutPlayerInfo = ActiveRoom.Players[PlayerIndex];
    return true;
}

void URTSGameInstance::ApplyNetworkRoomState(const FRTSRoomInfo& InRoomInfo)
{
    ActiveRoom = InRoomInfo;

    for (FRTSLobbyPlayerInfo& Player : ActiveRoom.Players)
    {
        Player.bIsLocalPlayer = Player.PlayerId == LocalLobbyPlayerId;
    }

    NormalizeActiveRoom();
    OnActiveRoomChanged.Broadcast(ActiveRoom);
}

void URTSGameInstance::ShowFlowWidgetForCurrentMap()
{
    ShowFlowWidgetForCurrentMap(GetWorld());
}

void URTSGameInstance::OpenConfiguredLevel(FName MapName) const
{
    if (MapName.IsNone() || !GetWorld())
    {
        return;
    }

    UGameplayStatics::OpenLevel(GetWorld(), MapName);
}

void URTSGameInstance::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
    ShowFlowWidgetForCurrentMap(LoadedWorld);
}

void URTSGameInstance::ShowFlowWidgetForCurrentMap(UWorld* World)
{
    if (!World)
    {
        RemoveActiveFlowWidget();
        return;
    }

    const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);

    auto DoesLevelMatch = [&CurrentLevelName](FName ConfiguredMapName)
    {
        return !ConfiguredMapName.IsNone()
            && CurrentLevelName.Equals(ConfiguredMapName.ToString(), ESearchCase::IgnoreCase);
    };

    if (DoesLevelMatch(MainMenuMapName))
    {
        ShowFlowWidget(MainMenuWidgetClass, World);
        return;
    }

    if (DoesLevelMatch(LobbyBrowserMapName))
    {
        ShowFlowWidget(LobbyBrowserWidgetClass, World);
        return;
    }

    if (DoesLevelMatch(LobbyMapName))
    {
        ShowFlowWidget(LobbyWidgetClass, World);
        return;
    }

    RemoveActiveFlowWidget();
}

void URTSGameInstance::BroadcastRoomStateChanged()
{
    NormalizeActiveRoom();
    RefreshLocalRoomList();
    OnActiveRoomChanged.Broadcast(ActiveRoom);
}

void URTSGameInstance::NormalizeActiveRoom()
{
    if (ActiveRoom.RoomId.IsNone())
    {
        ActiveRoom.CurrentPlayers = 0;
        ActiveRoom.bIsJoinable = false;
        bLocalPlayerIsRoomHost = false;
        return;
    }

    if (ActiveRoom.Players.Num() <= 0)
    {
        ActiveRoom = FRTSRoomInfo();
        bLocalPlayerIsRoomHost = false;
        return;
    }

    ActiveRoom.MaxPlayers = FMath::Max(1, ActiveRoom.MaxPlayers);
    ActiveRoom.CurrentPlayers = ActiveRoom.Players.Num();

    int32 HostIndex = INDEX_NONE;
    for (int32 PlayerIndex = 0; PlayerIndex < ActiveRoom.Players.Num(); ++PlayerIndex)
    {
        FRTSLobbyPlayerInfo& Player = ActiveRoom.Players[PlayerIndex];
        Player.bIsLocalPlayer = Player.PlayerId == LocalLobbyPlayerId;

        if (HostIndex == INDEX_NONE && (Player.bIsRoomHost || Player.PlayerId == ActiveRoom.HostPlayerId))
        {
            HostIndex = PlayerIndex;
        }
    }

    if (HostIndex == INDEX_NONE)
    {
        HostIndex = 0;
    }

    for (int32 PlayerIndex = 0; PlayerIndex < ActiveRoom.Players.Num(); ++PlayerIndex)
    {
        ActiveRoom.Players[PlayerIndex].bIsRoomHost = PlayerIndex == HostIndex;
    }

    const FRTSLobbyPlayerInfo& HostPlayer = ActiveRoom.Players[HostIndex];
    ActiveRoom.HostPlayerId = HostPlayer.PlayerId;
    ActiveRoom.HostName = HostPlayer.PlayerName;
    ActiveRoom.bIsJoinable = ActiveRoom.CurrentPlayers < ActiveRoom.MaxPlayers;

    bLocalPlayerIsRoomHost = HostPlayer.PlayerId == LocalLobbyPlayerId;
    SyncSelectedValuesFromLocalPlayer();
}

void URTSGameInstance::RemoveActiveFlowWidget()
{
    if (!ActiveFlowWidget)
    {
        return;
    }

    ActiveFlowWidget->RemoveFromParent();
    ActiveFlowWidget = nullptr;
}

void URTSGameInstance::ShowFlowWidget(TSubclassOf<UUserWidget> WidgetClass, UWorld* World)
{
    RemoveActiveFlowWidget();

    if (!WidgetClass || !World)
    {
        return;
    }

    APlayerController* PlayerController = World->GetFirstPlayerController();
    if (!PlayerController)
    {
        return;
    }

    UUserWidget* NewWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
    if (!NewWidget)
    {
        return;
    }

    ActiveFlowWidget = NewWidget;
    ActiveFlowWidget->AddToViewport();

    PlayerController->bShowMouseCursor = true;

    FInputModeUIOnly InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);
}

bool URTSGameInstance::CreateOnlineRoomSession()
{
    IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
    if (!SessionInterface)
    {
        return false;
    }

    if (SessionInterface->GetNamedSession(RTSLobbySession::SessionName))
    {
        bCreateSessionAfterDestroy = true;

        if (DestroySessionCompleteDelegateHandle.IsValid())
        {
            SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
            DestroySessionCompleteDelegateHandle.Reset();
        }

        DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
            FOnDestroySessionCompleteDelegate::CreateUObject(this, &URTSGameInstance::HandleDestroySessionComplete)
        );

        return SessionInterface->DestroySession(RTSLobbySession::SessionName);
    }

    FOnlineSessionSettings SessionSettings;
    SessionSettings.bIsLANMatch = true;
    SessionSettings.NumPublicConnections = FMath::Max(1, ActiveRoom.MaxPlayers);
    SessionSettings.bShouldAdvertise = true;
    SessionSettings.bAllowJoinInProgress = true;
    SessionSettings.bAllowJoinViaPresence = false;
    SessionSettings.bUsesPresence = false;

    SessionSettings.Set(RTSLobbySession::RoomIdKey, ActiveRoom.RoomId.ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    SessionSettings.Set(RTSLobbySession::RoomNameKey, ActiveRoom.RoomName.ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    SessionSettings.Set(RTSLobbySession::HostNameKey, ActiveRoom.HostName.ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    SessionSettings.Set(RTSLobbySession::MapNameKey, ActiveRoom.MapName.ToString(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    if (CreateSessionCompleteDelegateHandle.IsValid())
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
        CreateSessionCompleteDelegateHandle.Reset();
    }

    CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &URTSGameInstance::HandleCreateSessionComplete)
    );

    return SessionInterface->CreateSession(0, RTSLobbySession::SessionName, SessionSettings);
}

void URTSGameInstance::OpenLobbyAsListenServer()
{
    if (!GetWorld() || LobbyMapName.IsNone())
    {
        return;
    }

    UGameplayStatics::OpenLevel(GetWorld(), LobbyMapName, true, TEXT("listen"));
}

void URTSGameInstance::RefreshFallbackRoomList()
{
    NormalizeActiveRoom();

    CachedRooms.Reset();

    if (!ActiveRoom.RoomId.IsNone() && ActiveRoom.CurrentPlayers > 0)
    {
        CachedRooms.Add(ActiveRoom);
    }

    OnRoomListChanged.Broadcast();
}

bool URTSGameInstance::IsOnlineSessionAvailable() const
{
    return GetOnlineSessionInterface().IsValid();
}

IOnlineSessionPtr URTSGameInstance::GetOnlineSessionInterface() const
{
    IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
    return OnlineSubsystem
        ? OnlineSubsystem->GetSessionInterface()
        : nullptr;
}

ARTSLobbyPlayerController* URTSGameInstance::GetLobbyPlayerController() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    return Cast<ARTSLobbyPlayerController>(World->GetFirstPlayerController());
}

FRTSRoomInfo URTSGameInstance::MakeRoomInfoFromSessionResult(
    const FOnlineSessionSearchResult& SearchResult
) const
{
    FRTSRoomInfo RoomInfo;

    FString RoomIdString;
    FString RoomNameString;
    FString HostNameString;
    FString MapNameString;

    SearchResult.Session.SessionSettings.Get(RTSLobbySession::RoomIdKey, RoomIdString);
    SearchResult.Session.SessionSettings.Get(RTSLobbySession::RoomNameKey, RoomNameString);
    SearchResult.Session.SessionSettings.Get(RTSLobbySession::HostNameKey, HostNameString);
    SearchResult.Session.SessionSettings.Get(RTSLobbySession::MapNameKey, MapNameString);

    if (RoomIdString.IsEmpty())
    {
        RoomIdString = SearchResult.GetSessionIdStr();
    }

    RoomInfo.RoomId = FName(*RoomIdString);
    RoomInfo.RoomName = RoomNameString.IsEmpty()
        ? FText::FromString(TEXT("Room"))
        : FText::FromString(RoomNameString);
    RoomInfo.HostName = HostNameString.IsEmpty()
        ? FText::FromString(SearchResult.Session.OwningUserName)
        : FText::FromString(HostNameString);
    RoomInfo.MapName = MapNameString.IsEmpty()
        ? GameMapName
        : FName(*MapNameString);
    RoomInfo.MaxPlayers = FMath::Max(1, SearchResult.Session.SessionSettings.NumPublicConnections);
    RoomInfo.CurrentPlayers = FMath::Clamp(
        RoomInfo.MaxPlayers - SearchResult.Session.NumOpenPublicConnections,
        0,
        RoomInfo.MaxPlayers
    );
    RoomInfo.bIsJoinable = SearchResult.Session.NumOpenPublicConnections > 0;

    return RoomInfo;
}

const FOnlineSessionSearchResult* URTSGameInstance::FindSessionResultByRoomId(FName RoomId) const
{
    if (!RoomSessionSearch.IsValid())
    {
        return nullptr;
    }

    for (const FOnlineSessionSearchResult& SearchResult : RoomSessionSearch->SearchResults)
    {
        const FRTSRoomInfo RoomInfo = MakeRoomInfoFromSessionResult(SearchResult);
        if (RoomInfo.RoomId == RoomId)
        {
            return &SearchResult;
        }
    }

    return nullptr;
}

void URTSGameInstance::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
    if (SessionInterface && CreateSessionCompleteDelegateHandle.IsValid())
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
        CreateSessionCompleteDelegateHandle.Reset();
    }

    if (!bWasSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create RTS lobby session."));
        return;
    }

    OpenLobbyAsListenServer();
}

void URTSGameInstance::HandleFindSessionsComplete(bool bWasSuccessful)
{
    IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
    if (SessionInterface && FindSessionsCompleteDelegateHandle.IsValid())
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
        FindSessionsCompleteDelegateHandle.Reset();
    }

    CachedRooms.Reset();

    if (bWasSuccessful && RoomSessionSearch.IsValid())
    {
        for (const FOnlineSessionSearchResult& SearchResult : RoomSessionSearch->SearchResults)
        {
            CachedRooms.Add(MakeRoomInfoFromSessionResult(SearchResult));
        }
    }

    OnRoomListChanged.Broadcast();
}

void URTSGameInstance::HandleJoinSessionComplete(
    FName SessionName,
    EOnJoinSessionCompleteResult::Type Result
)
{
    IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
    if (SessionInterface && JoinSessionCompleteDelegateHandle.IsValid())
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
        JoinSessionCompleteDelegateHandle.Reset();
    }

    if (!SessionInterface || Result != EOnJoinSessionCompleteResult::Success)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to join RTS lobby session."));
        return;
    }

    FString ConnectString;
    if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString) || ConnectString.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to resolve RTS lobby session connect string."));
        return;
    }

    if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
    }
}

void URTSGameInstance::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSessionPtr SessionInterface = GetOnlineSessionInterface();
    if (SessionInterface && DestroySessionCompleteDelegateHandle.IsValid())
    {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
        DestroySessionCompleteDelegateHandle.Reset();
    }

    if (bCreateSessionAfterDestroy)
    {
        bCreateSessionAfterDestroy = false;
        CreateOnlineRoomSession();
        return;
    }

    ActiveRoom = FRTSRoomInfo();
    CachedRooms.Reset();
    bLocalPlayerIsRoomHost = false;
    OnRoomListChanged.Broadcast();
    OnActiveRoomChanged.Broadcast(ActiveRoom);
    OpenLobbyBrowser();
}

FName URTSGameInstance::GetOrCreateLocalLobbyPlayerId()
{
    if (LocalLobbyPlayerId.IsNone())
    {
        LocalLobbyPlayerId = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
    }

    return LocalLobbyPlayerId;
}

FName URTSGameInstance::ResolveGameMapName(FName RequestedMapName) const
{
    if (!RequestedMapName.IsNone())
    {
        return RequestedMapName;
    }

    if (!SelectedGameMapName.IsNone())
    {
        return SelectedGameMapName;
    }

    return GameMapName;
}

FText URTSGameInstance::MakeDefaultPlayerName() const
{
    if (!LocalLobbyPlayerName.IsEmpty())
    {
        return LocalLobbyPlayerName;
    }

    return FText::FromString(TEXT("Player"));
}

FRTSLobbyPlayerInfo URTSGameInstance::MakeLocalLobbyPlayerInfo(const FText& PlayerName, bool bRoomHost)
{
    FRTSLobbyPlayerInfo PlayerInfo;
    PlayerInfo.PlayerId = GetOrCreateLocalLobbyPlayerId();
    PlayerInfo.PlayerName = PlayerName.IsEmpty() ? MakeDefaultPlayerName() : PlayerName;
    PlayerInfo.TeamNumber = bRoomHost ? ClampLobbyTeamNumber(SelectedTeamNumber) : GetBalancedTeamNumber();
    PlayerInfo.Race = SelectedRace;
    PlayerInfo.PlayerColor = IsPlayerColorAvailable(SelectedPlayerColor, PlayerInfo.PlayerId)
        ? SelectedPlayerColor
        : GetFirstAvailablePlayerColor(PlayerInfo.PlayerId);
    PlayerInfo.bIsRoomHost = bRoomHost;
    PlayerInfo.bIsLocalPlayer = true;

    SelectedTeamNumber = PlayerInfo.TeamNumber;
    SelectedPlayerColor = PlayerInfo.PlayerColor;
    LocalLobbyPlayerName = PlayerInfo.PlayerName;

    return PlayerInfo;
}

int32 URTSGameInstance::FindLobbyPlayerIndex(FName PlayerId) const
{
    if (PlayerId.IsNone())
    {
        return INDEX_NONE;
    }

    for (int32 PlayerIndex = 0; PlayerIndex < ActiveRoom.Players.Num(); ++PlayerIndex)
    {
        if (ActiveRoom.Players[PlayerIndex].PlayerId == PlayerId)
        {
            return PlayerIndex;
        }
    }

    return INDEX_NONE;
}

int32 URTSGameInstance::FindLocalLobbyPlayerIndex() const
{
    return FindLobbyPlayerIndex(LocalLobbyPlayerId);
}

int32 URTSGameInstance::ClampLobbyTeamNumber(int32 InTeamNumber) const
{
    const int32 MinTeam = FMath::Min(MinLobbyTeamNumber, MaxLobbyTeamNumber);
    const int32 MaxTeam = FMath::Max(MinLobbyTeamNumber, MaxLobbyTeamNumber);
    return FMath::Clamp(InTeamNumber, MinTeam, MaxTeam);
}

int32 URTSGameInstance::GetBalancedTeamNumber() const
{
    int32 BestTeamNumber = ClampLobbyTeamNumber(MinLobbyTeamNumber);
    int32 BestTeamCount = MAX_int32;

    const int32 MinTeam = FMath::Min(MinLobbyTeamNumber, MaxLobbyTeamNumber);
    const int32 MaxTeam = FMath::Max(MinLobbyTeamNumber, MaxLobbyTeamNumber);

    for (int32 TeamNumber = MinTeam; TeamNumber <= MaxTeam; ++TeamNumber)
    {
        int32 TeamCount = 0;
        for (const FRTSLobbyPlayerInfo& Player : ActiveRoom.Players)
        {
            if (Player.TeamNumber == TeamNumber)
            {
                ++TeamCount;
            }
        }

        if (TeamCount < BestTeamCount)
        {
            BestTeamCount = TeamCount;
            BestTeamNumber = TeamNumber;
        }
    }

    return BestTeamNumber;
}

bool URTSGameInstance::CanEditLobbyPlayer(FName PlayerId) const
{
    return PlayerId == LocalLobbyPlayerId || IsLocalPlayerRoomHost();
}

bool URTSGameInstance::AreLobbyColorsEqual(const FLinearColor& A, const FLinearColor& B) const
{
    constexpr float ColorTolerance = 0.01f;

    return FMath::Abs(A.R - B.R) <= ColorTolerance
        && FMath::Abs(A.G - B.G) <= ColorTolerance
        && FMath::Abs(A.B - B.B) <= ColorTolerance
        && FMath::Abs(A.A - B.A) <= ColorTolerance;
}

void URTSGameInstance::SyncSelectedValuesFromLocalPlayer()
{
    const int32 LocalPlayerIndex = FindLocalLobbyPlayerIndex();
    if (LocalPlayerIndex == INDEX_NONE)
    {
        return;
    }

    const FRTSLobbyPlayerInfo& LocalPlayer = ActiveRoom.Players[LocalPlayerIndex];
    SelectedTeamNumber = LocalPlayer.TeamNumber;
    SelectedRace = LocalPlayer.Race;
    SelectedPlayerColor = LocalPlayer.PlayerColor;
    LocalLobbyPlayerName = LocalPlayer.PlayerName;
}
