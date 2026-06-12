#include "Core/RTSGameInstance.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/Guid.h"

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
    OpenConfiguredLevel(GameMapName);
}

void URTSGameInstance::RefreshLocalRoomList()
{
    CachedRooms.Reset();

    if (!ActiveRoom.RoomId.IsNone() && ActiveRoom.bIsJoinable)
    {
        CachedRooms.Add(ActiveRoom);
    }

    OnRoomListChanged.Broadcast();
}

bool URTSGameInstance::HostLocalRoom(const FText& RoomName, const FText& HostName)
{
    bLocalPlayerIsRoomHost = true;

    const FText EffectiveRoomName = RoomName.IsEmpty()
        ? FText::FromString(TEXT("Local Room"))
        : RoomName;

    const FText EffectiveHostName = HostName.IsEmpty()
        ? FText::FromString(TEXT("Player"))
        : HostName;

    ActiveRoom.RoomId = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
    ActiveRoom.RoomName = EffectiveRoomName;
    ActiveRoom.HostName = EffectiveHostName;
    ActiveRoom.MapName = GameMapName;
    ActiveRoom.CurrentPlayers = 1;
    ActiveRoom.MaxPlayers = FMath::Max(1, DefaultMaxPlayers);
    ActiveRoom.bIsJoinable = true;

    RefreshLocalRoomList();
    OpenLobby();
    return true;
}

bool URTSGameInstance::JoinLocalRoom(FName RoomId)
{
    bLocalPlayerIsRoomHost = false;

    RefreshLocalRoomList();

    for (const FRTSRoomInfo& Room : CachedRooms)
    {
        if (Room.RoomId == RoomId && Room.bIsJoinable)
        {
            ActiveRoom = Room;
            OpenLobby();
            return true;
        }
    }

    return false;
}

bool URTSGameInstance::IsLocalPlayerRoomHost() const
{
    return bLocalPlayerIsRoomHost;
}

void URTSGameInstance::SetSelectedRace(ERTSRace InRace)
{
    SelectedRace = InRace;
}

void URTSGameInstance::SetSelectedPlayerColor(FLinearColor InColor)
{
    SelectedPlayerColor = InColor;
}

void URTSGameInstance::OpenConfiguredLevel(FName MapName) const
{
    if (MapName.IsNone() || !GetWorld())
    {
        return;
    }

    UGameplayStatics::OpenLevel(GetWorld(), MapName);
}
