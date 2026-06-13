#include "UI/RTSLobbyPlayerSlotWidget.h"

#include "UI/RTSLobbyWidget.h"

void URTSLobbyPlayerSlotWidget::SetupPlayerSlot(
    const FRTSLobbyPlayerInfo& InPlayerInfo,
    URTSLobbyWidget* InOwnerLobbyWidget
)
{
    OwnerLobbyWidget = InOwnerLobbyWidget;
    UpdatePlayerInfo(InPlayerInfo);
}

void URTSLobbyPlayerSlotWidget::UpdatePlayerInfo(const FRTSLobbyPlayerInfo& InPlayerInfo)
{
    PlayerInfo = InPlayerInfo;
    OnPlayerInfoUpdated(PlayerInfo);
}

void URTSLobbyPlayerSlotWidget::SetOwnerLobbyWidget(URTSLobbyWidget* InOwnerLobbyWidget)
{
    OwnerLobbyWidget = InOwnerLobbyWidget;
}

bool URTSLobbyPlayerSlotWidget::TrySetTeam(int32 InTeamNumber)
{
    const bool bChanged = OwnerLobbyWidget
        && CanLocalPlayerChangeTeam()
        && OwnerLobbyWidget->SetPlayerTeam(PlayerInfo.PlayerId, InTeamNumber);

    if (!bChanged)
    {
        OnPlayerActionRejected(PlayerInfo);
    }

    return bChanged;
}

bool URTSLobbyPlayerSlotWidget::TrySetRace(ERTSRace InRace)
{
    const bool bChanged = OwnerLobbyWidget
        && CanLocalPlayerChangeRace()
        && OwnerLobbyWidget->SetPlayerRace(PlayerInfo.PlayerId, InRace);

    if (!bChanged)
    {
        OnPlayerActionRejected(PlayerInfo);
    }

    return bChanged;
}

bool URTSLobbyPlayerSlotWidget::TrySetPlayerColor(FLinearColor InColor)
{
    const bool bChanged = OwnerLobbyWidget
        && CanLocalPlayerChangeColor()
        && OwnerLobbyWidget->SetPlayerColor(PlayerInfo.PlayerId, InColor);

    if (!bChanged)
    {
        OnPlayerActionRejected(PlayerInfo);
    }

    return bChanged;
}

bool URTSLobbyPlayerSlotWidget::IsLocalPlayerSlot() const
{
    return PlayerInfo.bIsLocalPlayer;
}

bool URTSLobbyPlayerSlotWidget::IsRoomHostSlot() const
{
    return PlayerInfo.bIsRoomHost;
}

bool URTSLobbyPlayerSlotWidget::CanLocalPlayerEditSlot() const
{
    return OwnerLobbyWidget
        && (PlayerInfo.bIsLocalPlayer || OwnerLobbyWidget->IsLocalPlayerRoomHost());
}

bool URTSLobbyPlayerSlotWidget::CanLocalPlayerChangeTeam() const
{
    return CanLocalPlayerEditSlot();
}

bool URTSLobbyPlayerSlotWidget::CanLocalPlayerChangeRace() const
{
    return CanLocalPlayerEditSlot();
}

bool URTSLobbyPlayerSlotWidget::CanLocalPlayerChangeColor() const
{
    return CanLocalPlayerEditSlot();
}
