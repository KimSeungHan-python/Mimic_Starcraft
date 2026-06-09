#include "UI/RTSLobbyWidget.h"

void URTSLobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BindToGameInstance(GetGameInstance<URTSGameInstance>());
}

void URTSLobbyWidget::BindToGameInstance(URTSGameInstance* InGameInstance)
{
    RTSGameInstance = InGameInstance;
    RefreshActiveRoom();
}

void URTSLobbyWidget::RefreshActiveRoom()
{
    ActiveRoom = RTSGameInstance
        ? RTSGameInstance->ActiveRoom
        : FRTSRoomInfo();

    OnActiveRoomUpdated(ActiveRoom);
}

void URTSLobbyWidget::StartGame()
{
    if (RTSGameInstance)
    {
        RTSGameInstance->StartGameFromLobby();
    }
}

void URTSLobbyWidget::ReturnToRoomBrowser()
{
    if (RTSGameInstance)
    {
        RTSGameInstance->OpenLobbyBrowser();
    }
}
