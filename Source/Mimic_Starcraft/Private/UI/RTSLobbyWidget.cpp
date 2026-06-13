#include "UI/RTSLobbyWidget.h"

void URTSLobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BindToGameInstance(GetGameInstance<URTSGameInstance>());
}

void URTSLobbyWidget::NativeDestruct()
{
    if (RTSGameInstance)
    {
        RTSGameInstance->OnActiveRoomChanged.RemoveDynamic(this, &URTSLobbyWidget::HandleActiveRoomChanged);
    }

    Super::NativeDestruct();
}

void URTSLobbyWidget::BindToGameInstance(URTSGameInstance* InGameInstance)
{
    if (RTSGameInstance == InGameInstance)
    {
        RefreshActiveRoom();
        return;
    }

    if (RTSGameInstance)
    {
        RTSGameInstance->OnActiveRoomChanged.RemoveDynamic(this, &URTSLobbyWidget::HandleActiveRoomChanged);
    }

    RTSGameInstance = InGameInstance;

    if (RTSGameInstance)
    {
        RTSGameInstance->OnActiveRoomChanged.AddUniqueDynamic(this, &URTSLobbyWidget::HandleActiveRoomChanged);
    }

    RefreshActiveRoom();
}

void URTSLobbyWidget::RefreshActiveRoom()
{
    ActiveRoom = RTSGameInstance
        ? RTSGameInstance->ActiveRoom
        : FRTSRoomInfo();

    OnActiveRoomUpdated(ActiveRoom);
}

void URTSLobbyWidget::HandleActiveRoomChanged(const FRTSRoomInfo& UpdatedRoom)
{
    ActiveRoom = UpdatedRoom;
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

FName URTSLobbyWidget::GetLocalLobbyPlayerId() const
{
    return RTSGameInstance
        ? RTSGameInstance->GetLocalLobbyPlayerId()
        : NAME_None;
}

bool URTSLobbyWidget::SetSelectedTeamNumber(int32 InTeamNumber)
{
    return RTSGameInstance
        ? RTSGameInstance->SetSelectedTeamNumber(InTeamNumber)
        : false;
}

bool URTSLobbyWidget::SetPlayerTeam(FName PlayerId, int32 InTeamNumber)
{
    return RTSGameInstance
        ? RTSGameInstance->SetLobbyPlayerTeam(PlayerId, InTeamNumber)
        : false;
}

bool URTSLobbyWidget::SetPlayerTeamByIndex(int32 PlayerIndex, int32 InTeamNumber)
{
    return RTSGameInstance
        ? RTSGameInstance->SetLobbyPlayerTeamByIndex(PlayerIndex, InTeamNumber)
        : false;
}

void URTSLobbyWidget::SetSelectedRace(ERTSRace InRace)
{
    if (RTSGameInstance)
    {
        RTSGameInstance->SetSelectedRace(InRace);
    }
}

bool URTSLobbyWidget::SetPlayerRace(FName PlayerId, ERTSRace InRace)
{
    return RTSGameInstance
        ? RTSGameInstance->SetLobbyPlayerRace(PlayerId, InRace)
        : false;
}

void URTSLobbyWidget::SetSelectedPlayerColor(FLinearColor InColor)
{
    if (RTSGameInstance)
    {
        RTSGameInstance->SetSelectedPlayerColor(InColor);
    }
}

bool URTSLobbyWidget::TrySetSelectedPlayerColor(FLinearColor InColor)
{
    return RTSGameInstance
        ? RTSGameInstance->TrySetSelectedPlayerColor(InColor)
        : false;
}

bool URTSLobbyWidget::SetPlayerColor(FName PlayerId, FLinearColor InColor)
{
    return RTSGameInstance
        ? RTSGameInstance->SetLobbyPlayerColor(PlayerId, InColor)
        : false;
}

bool URTSLobbyWidget::LeaveRoom()
{
    return RTSGameInstance
        ? RTSGameInstance->LeaveActiveRoom()
        : false;
}

void URTSLobbyWidget::ReturnToRoomBrowser()
{
    if (RTSGameInstance)
    {
        RTSGameInstance->LeaveActiveRoom();
    }
}
