#include "UI/RTSLobbyBrowserWidget.h"

void URTSLobbyBrowserWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BindToGameInstance(GetGameInstance<URTSGameInstance>());
}

void URTSLobbyBrowserWidget::NativeDestruct()
{
    if (RTSGameInstance)
    {
        RTSGameInstance->OnRoomListChanged.RemoveDynamic(this, &URTSLobbyBrowserWidget::RefreshRooms);
    }

    Super::NativeDestruct();
}

void URTSLobbyBrowserWidget::BindToGameInstance(URTSGameInstance* InGameInstance)
{
    if (RTSGameInstance == InGameInstance)
    {
        RefreshRooms();
        return;
    }

    if (RTSGameInstance)
    {
        RTSGameInstance->OnRoomListChanged.RemoveDynamic(this, &URTSLobbyBrowserWidget::RefreshRooms);
    }

    RTSGameInstance = InGameInstance;

    if (RTSGameInstance)
    {
        RTSGameInstance->OnRoomListChanged.AddUniqueDynamic(this, &URTSLobbyBrowserWidget::RefreshRooms);
        RTSGameInstance->RefreshLocalRoomList();
        return;
    }

    RefreshRooms();
}

void URTSLobbyBrowserWidget::RefreshRooms()
{
    Rooms.Reset();

    if (RTSGameInstance)
    {
        Rooms = RTSGameInstance->CachedRooms;
    }

    OnRoomsUpdated(Rooms);
}

bool URTSLobbyBrowserWidget::HostRoom(const FText& RoomName, const FText& HostName)
{
    return RTSGameInstance
        ? RTSGameInstance->HostLocalRoom(RoomName, HostName)
        : false;
}

bool URTSLobbyBrowserWidget::JoinRoom(FName RoomId)
{
    return RTSGameInstance
        ? RTSGameInstance->JoinLocalRoom(RoomId)
        : false;
}

void URTSLobbyBrowserWidget::OpenMainMenu()
{
    if (RTSGameInstance)
    {
        RTSGameInstance->OpenMainMenu();
    }
}
