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

bool URTSLobbyWidget::StartGame()
{
    if (!RTSGameInstance)
    {
        return false;
    }

    if (!RTSGameInstance->IsLocalPlayerRoomHost())
    {
        UE_LOG(LogTemp, Warning, TEXT("Only room host can start the game."));
        return false;
    }

    RTSGameInstance->StartGameFromLobby();
    return true;
}

bool URTSLobbyWidget::IsLocalPlayerRoomHost() const
{
    return RTSGameInstance
        ? RTSGameInstance->IsLocalPlayerRoomHost()
        : false;
}

void URTSLobbyWidget::SetSelectedRace(ERTSRace InRace)
{
    if (RTSGameInstance)
    {
        RTSGameInstance->SetSelectedRace(InRace);
    }
}

void URTSLobbyWidget::SetSelectedPlayerColor(FLinearColor InColor)
{
    if (RTSGameInstance)
    {
        RTSGameInstance->SetSelectedPlayerColor(InColor);
    }
}

void URTSLobbyWidget::ReturnToRoomBrowser()
{
    if (RTSGameInstance)
    {
        RTSGameInstance->OpenLobbyBrowser();
    }
}
