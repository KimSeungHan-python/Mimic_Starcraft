#include "Core/RTSLobbyGameMode.h"

#include "Core/RTSGameInstance.h"
#include "Core/RTSLobbyGameState.h"
#include "Core/RTSLobbyPlayerController.h"
#include "Misc/Guid.h"

ARTSLobbyGameMode::ARTSLobbyGameMode()
{
    GameStateClass = ARTSLobbyGameState::StaticClass();
    PlayerControllerClass = ARTSLobbyPlayerController::StaticClass();
}

void ARTSLobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

    InitializeRoomIfNeeded();
}

void ARTSLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    InitializeRoomIfNeeded();

    if (ARTSLobbyPlayerController* LobbyPlayerController = Cast<ARTSLobbyPlayerController>(NewPlayer))
    {
        LobbyPlayerController->ClientRequestLobbyRegistration();
    }
}

void ARTSLobbyGameMode::Logout(AController* Exiting)
{
    if (ARTSLobbyPlayerController* LobbyPlayerController = Cast<ARTSLobbyPlayerController>(Exiting))
    {
        if (!LobbyPlayerController->LobbyPlayerId.IsNone())
        {
            FRTSRoomInfo RoomInfo = GetMutableRoom();
            const int32 PlayerIndex = FindPlayerIndex(RoomInfo, LobbyPlayerController->LobbyPlayerId);
            if (PlayerIndex != INDEX_NONE)
            {
                RoomInfo.Players.RemoveAt(PlayerIndex);
                NormalizeRoom(RoomInfo);
                PublishRoom(RoomInfo);
            }
        }
    }

    Super::Logout(Exiting);
}

bool ARTSLobbyGameMode::RegisterLobbyPlayer(
    ARTSLobbyPlayerController* PlayerController,
    FName PlayerId,
    const FText& PlayerName,
    int32 TeamNumber,
    ERTSRace Race,
    const FLinearColor& PlayerColor
)
{
    if (!PlayerController)
    {
        return false;
    }

    InitializeRoomIfNeeded();

    FRTSRoomInfo RoomInfo = GetMutableRoom();
    if (PlayerId.IsNone())
    {
        PlayerId = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
    }

    const int32 ExistingPlayerIndex = FindPlayerIndex(RoomInfo, PlayerId);
    FRTSLobbyPlayerInfo PlayerInfo;

    if (ExistingPlayerIndex != INDEX_NONE)
    {
        PlayerInfo = RoomInfo.Players[ExistingPlayerIndex];
    }

    PlayerInfo.PlayerId = PlayerId;
    PlayerInfo.PlayerName = PlayerName.IsEmpty()
        ? FText::FromString(TEXT("Player"))
        : PlayerName;
    PlayerInfo.TeamNumber = TeamNumber > 0
        ? ClampTeamNumber(TeamNumber)
        : GetBalancedTeamNumber(RoomInfo);
    PlayerInfo.Race = Race;
    PlayerInfo.PlayerColor = IsColorAvailable(RoomInfo, PlayerColor, PlayerId)
        ? PlayerColor
        : GetFirstAvailableColor(RoomInfo, PlayerId);
    PlayerInfo.bIsRoomHost = RoomInfo.Players.Num() == 0 || RoomInfo.HostPlayerId == PlayerId;
    PlayerInfo.bIsLocalPlayer = false;

    if (ExistingPlayerIndex != INDEX_NONE)
    {
        RoomInfo.Players[ExistingPlayerIndex] = PlayerInfo;
    }
    else if (RoomInfo.Players.Num() < RoomInfo.MaxPlayers)
    {
        RoomInfo.Players.Add(PlayerInfo);
    }
    else
    {
        PlayerController->ClientReturnToLobbyBrowser();
        return false;
    }

    PlayerController->LobbyPlayerId = PlayerId;

    NormalizeRoom(RoomInfo);
    PublishRoom(RoomInfo);
    return true;
}

bool ARTSLobbyGameMode::SetLobbyPlayerTeam(
    ARTSLobbyPlayerController* RequestingPlayer,
    FName TargetPlayerId,
    int32 TeamNumber
)
{
    if (!RequestingPlayer)
    {
        return false;
    }

    FRTSRoomInfo RoomInfo = GetMutableRoom();
    if (!CanEditPlayer(RoomInfo, RequestingPlayer->LobbyPlayerId, TargetPlayerId))
    {
        return false;
    }

    const int32 PlayerIndex = FindPlayerIndex(RoomInfo, TargetPlayerId);
    if (PlayerIndex == INDEX_NONE)
    {
        return false;
    }

    RoomInfo.Players[PlayerIndex].TeamNumber = ClampTeamNumber(TeamNumber);
    NormalizeRoom(RoomInfo);
    PublishRoom(RoomInfo);
    return true;
}

bool ARTSLobbyGameMode::SetLobbyPlayerName(
    ARTSLobbyPlayerController* RequestingPlayer,
    FName TargetPlayerId,
    const FText& PlayerName
)
{
    if (!RequestingPlayer)
    {
        return false;
    }

    FRTSRoomInfo RoomInfo = GetMutableRoom();
    if (!CanEditPlayer(RoomInfo, RequestingPlayer->LobbyPlayerId, TargetPlayerId))
    {
        return false;
    }

    const int32 PlayerIndex = FindPlayerIndex(RoomInfo, TargetPlayerId);
    if (PlayerIndex == INDEX_NONE)
    {
        return false;
    }

    RoomInfo.Players[PlayerIndex].PlayerName = PlayerName.IsEmpty()
        ? FText::FromString(TEXT("Player"))
        : PlayerName;
    NormalizeRoom(RoomInfo);
    PublishRoom(RoomInfo);
    return true;
}

bool ARTSLobbyGameMode::SetLobbyPlayerRace(
    ARTSLobbyPlayerController* RequestingPlayer,
    FName TargetPlayerId,
    ERTSRace Race
)
{
    if (!RequestingPlayer)
    {
        return false;
    }

    FRTSRoomInfo RoomInfo = GetMutableRoom();
    if (!CanEditPlayer(RoomInfo, RequestingPlayer->LobbyPlayerId, TargetPlayerId))
    {
        return false;
    }

    const int32 PlayerIndex = FindPlayerIndex(RoomInfo, TargetPlayerId);
    if (PlayerIndex == INDEX_NONE)
    {
        return false;
    }

    RoomInfo.Players[PlayerIndex].Race = Race;
    NormalizeRoom(RoomInfo);
    PublishRoom(RoomInfo);
    return true;
}

bool ARTSLobbyGameMode::SetLobbyPlayerColor(
    ARTSLobbyPlayerController* RequestingPlayer,
    FName TargetPlayerId,
    const FLinearColor& PlayerColor
)
{
    if (!RequestingPlayer)
    {
        return false;
    }

    FRTSRoomInfo RoomInfo = GetMutableRoom();
    if (!CanEditPlayer(RoomInfo, RequestingPlayer->LobbyPlayerId, TargetPlayerId)
        || !IsColorAvailable(RoomInfo, PlayerColor, TargetPlayerId))
    {
        return false;
    }

    const int32 PlayerIndex = FindPlayerIndex(RoomInfo, TargetPlayerId);
    if (PlayerIndex == INDEX_NONE)
    {
        return false;
    }

    RoomInfo.Players[PlayerIndex].PlayerColor = PlayerColor;
    NormalizeRoom(RoomInfo);
    PublishRoom(RoomInfo);
    return true;
}

bool ARTSLobbyGameMode::StartLobbyGame(ARTSLobbyPlayerController* RequestingPlayer)
{
    if (!RequestingPlayer)
    {
        return false;
    }

    const FRTSRoomInfo RoomInfo = GetMutableRoom();
    if (!IsRoomHost(RoomInfo, RequestingPlayer->LobbyPlayerId))
    {
        return false;
    }

    const FName MapName = RoomInfo.MapName.IsNone()
        ? FName(TEXT("GPTTestMap"))
        : RoomInfo.MapName;

    if (UWorld* World = GetWorld())
    {
        World->ServerTravel(MapName.ToString() + TEXT("?listen"));
        return true;
    }

    return false;
}

bool ARTSLobbyGameMode::LeaveLobby(ARTSLobbyPlayerController* RequestingPlayer)
{
    if (!RequestingPlayer)
    {
        return false;
    }

    FRTSRoomInfo RoomInfo = GetMutableRoom();
    const int32 PlayerIndex = FindPlayerIndex(RoomInfo, RequestingPlayer->LobbyPlayerId);
    if (PlayerIndex != INDEX_NONE)
    {
        RoomInfo.Players.RemoveAt(PlayerIndex);
        NormalizeRoom(RoomInfo);
        PublishRoom(RoomInfo);
    }

    RequestingPlayer->ClientReturnToLobbyBrowser();
    return true;
}

void ARTSLobbyGameMode::InitializeRoomIfNeeded()
{
    ARTSLobbyGameState* LobbyGameState = GetLobbyGameState();
    if (!LobbyGameState || !LobbyGameState->ActiveRoom.RoomId.IsNone())
    {
        return;
    }

    FRTSRoomInfo RoomInfo;
    if (URTSGameInstance* RTSGameInstance = GetGameInstance<URTSGameInstance>())
    {
        RoomInfo = RTSGameInstance->ActiveRoom;
        MinLobbyTeamNumber = RTSGameInstance->MinLobbyTeamNumber;
        MaxLobbyTeamNumber = RTSGameInstance->MaxLobbyTeamNumber;
    }

    if (RoomInfo.RoomId.IsNone())
    {
        RoomInfo.RoomId = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
    }

    if (RoomInfo.RoomName.IsEmpty())
    {
        RoomInfo.RoomName = FText::FromString(TEXT("Lobby"));
    }

    if (RoomInfo.MapName.IsNone())
    {
        RoomInfo.MapName = FName(TEXT("GPTTestMap"));
    }

    RoomInfo.MaxPlayers = FMath::Max(1, RoomInfo.MaxPlayers);
    RoomInfo.Players.Reset();
    NormalizeRoom(RoomInfo);
    PublishRoom(RoomInfo);
}

void ARTSLobbyGameMode::PublishRoom(const FRTSRoomInfo& InRoomInfo)
{
    if (ARTSLobbyGameState* LobbyGameState = GetLobbyGameState())
    {
        LobbyGameState->SetActiveRoom(InRoomInfo);
    }

    if (URTSGameInstance* RTSGameInstance = GetGameInstance<URTSGameInstance>())
    {
        RTSGameInstance->UpdateAdvertisedRoomSession(InRoomInfo);
    }
}

FRTSRoomInfo ARTSLobbyGameMode::GetMutableRoom() const
{
    if (const ARTSLobbyGameState* LobbyGameState = GetLobbyGameState())
    {
        return LobbyGameState->ActiveRoom;
    }

    return FRTSRoomInfo();
}

int32 ARTSLobbyGameMode::FindPlayerIndex(const FRTSRoomInfo& RoomInfo, FName PlayerId) const
{
    if (PlayerId.IsNone())
    {
        return INDEX_NONE;
    }

    for (int32 PlayerIndex = 0; PlayerIndex < RoomInfo.Players.Num(); ++PlayerIndex)
    {
        if (RoomInfo.Players[PlayerIndex].PlayerId == PlayerId)
        {
            return PlayerIndex;
        }
    }

    return INDEX_NONE;
}

int32 ARTSLobbyGameMode::ClampTeamNumber(int32 TeamNumber) const
{
    const int32 MinTeam = FMath::Min(MinLobbyTeamNumber, MaxLobbyTeamNumber);
    const int32 MaxTeam = FMath::Max(MinLobbyTeamNumber, MaxLobbyTeamNumber);
    return FMath::Clamp(TeamNumber, MinTeam, MaxTeam);
}

int32 ARTSLobbyGameMode::GetBalancedTeamNumber(const FRTSRoomInfo& RoomInfo) const
{
    int32 BestTeamNumber = ClampTeamNumber(MinLobbyTeamNumber);
    int32 BestTeamCount = MAX_int32;

    const int32 MinTeam = FMath::Min(MinLobbyTeamNumber, MaxLobbyTeamNumber);
    const int32 MaxTeam = FMath::Max(MinLobbyTeamNumber, MaxLobbyTeamNumber);

    for (int32 TeamNumber = MinTeam; TeamNumber <= MaxTeam; ++TeamNumber)
    {
        int32 TeamCount = 0;
        for (const FRTSLobbyPlayerInfo& Player : RoomInfo.Players)
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

bool ARTSLobbyGameMode::CanEditPlayer(
    const FRTSRoomInfo& RoomInfo,
    FName RequestingPlayerId,
    FName TargetPlayerId
) const
{
    return RequestingPlayerId == TargetPlayerId || IsRoomHost(RoomInfo, RequestingPlayerId);
}

bool ARTSLobbyGameMode::IsRoomHost(const FRTSRoomInfo& RoomInfo, FName PlayerId) const
{
    return !PlayerId.IsNone() && RoomInfo.HostPlayerId == PlayerId;
}

bool ARTSLobbyGameMode::IsColorAvailable(
    const FRTSRoomInfo& RoomInfo,
    const FLinearColor& PlayerColor,
    FName IgnoredPlayerId
) const
{
    constexpr float ColorTolerance = 0.01f;

    for (const FRTSLobbyPlayerInfo& Player : RoomInfo.Players)
    {
        if (Player.PlayerId == IgnoredPlayerId)
        {
            continue;
        }

        const bool bSameColor =
            FMath::Abs(Player.PlayerColor.R - PlayerColor.R) <= ColorTolerance
            && FMath::Abs(Player.PlayerColor.G - PlayerColor.G) <= ColorTolerance
            && FMath::Abs(Player.PlayerColor.B - PlayerColor.B) <= ColorTolerance
            && FMath::Abs(Player.PlayerColor.A - PlayerColor.A) <= ColorTolerance;

        if (bSameColor)
        {
            return false;
        }
    }

    return true;
}

FLinearColor ARTSLobbyGameMode::GetFirstAvailableColor(const FRTSRoomInfo& RoomInfo, FName IgnoredPlayerId) const
{
    const TArray<FLinearColor> CandidateColors =
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

    for (const FLinearColor& CandidateColor : CandidateColors)
    {
        if (IsColorAvailable(RoomInfo, CandidateColor, IgnoredPlayerId))
        {
            return CandidateColor;
        }
    }

    return FLinearColor::White;
}

void ARTSLobbyGameMode::NormalizeRoom(FRTSRoomInfo& RoomInfo) const
{
    RoomInfo.MaxPlayers = FMath::Max(1, RoomInfo.MaxPlayers);
    RoomInfo.CurrentPlayers = RoomInfo.Players.Num();
    RoomInfo.bIsJoinable = RoomInfo.CurrentPlayers < RoomInfo.MaxPlayers;

    int32 HostIndex = FindPlayerIndex(RoomInfo, RoomInfo.HostPlayerId);
    if (HostIndex == INDEX_NONE && RoomInfo.Players.Num() > 0)
    {
        HostIndex = 0;
        RoomInfo.HostPlayerId = RoomInfo.Players[HostIndex].PlayerId;
    }

    for (int32 PlayerIndex = 0; PlayerIndex < RoomInfo.Players.Num(); ++PlayerIndex)
    {
        FRTSLobbyPlayerInfo& Player = RoomInfo.Players[PlayerIndex];
        Player.bIsRoomHost = PlayerIndex == HostIndex;
        Player.bIsLocalPlayer = false;
    }

    if (RoomInfo.Players.IsValidIndex(HostIndex))
    {
        RoomInfo.HostName = RoomInfo.Players[HostIndex].PlayerName;
    }
    else
    {
        RoomInfo.HostName = FText::GetEmpty();
        RoomInfo.HostPlayerId = NAME_None;
    }
}

ARTSLobbyGameState* ARTSLobbyGameMode::GetLobbyGameState() const
{
    return GetGameState<ARTSLobbyGameState>();
}
