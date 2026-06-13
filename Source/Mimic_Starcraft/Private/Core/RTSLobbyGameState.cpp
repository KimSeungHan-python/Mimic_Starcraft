#include "Core/RTSLobbyGameState.h"

#include "Net/UnrealNetwork.h"

ARTSLobbyGameState::ARTSLobbyGameState()
{
    bReplicates = true;
}

void ARTSLobbyGameState::SetActiveRoom(const FRTSRoomInfo& InRoomInfo)
{
    if (!HasAuthority())
    {
        return;
    }

    ActiveRoom = InRoomInfo;
    BroadcastActiveRoomChanged();
}

void ARTSLobbyGameState::OnRep_ActiveRoom()
{
    BroadcastActiveRoomChanged();
}

void ARTSLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARTSLobbyGameState, ActiveRoom);
}

void ARTSLobbyGameState::BroadcastActiveRoomChanged()
{
    if (URTSGameInstance* RTSGameInstance = GetGameInstance<URTSGameInstance>())
    {
        RTSGameInstance->ApplyNetworkRoomState(ActiveRoom);
    }

    OnLobbyRoomChanged.Broadcast(ActiveRoom);
}
