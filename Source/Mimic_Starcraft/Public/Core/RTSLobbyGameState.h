#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/RTSGameInstance.h"
#include "RTSLobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTSLobbyReplicatedRoomChanged, const FRTSRoomInfo&, UpdatedRoom);

UCLASS()
class MIMIC_STARCRAFT_API ARTSLobbyGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ARTSLobbyGameState();

    UPROPERTY(ReplicatedUsing = OnRep_ActiveRoom, BlueprintReadOnly, Category = "RTS Lobby")
    FRTSRoomInfo ActiveRoom;

    UPROPERTY(BlueprintAssignable, Category = "RTS Lobby")
    FRTSLobbyReplicatedRoomChanged OnLobbyRoomChanged;

    void SetActiveRoom(const FRTSRoomInfo& InRoomInfo);

    UFUNCTION()
    void OnRep_ActiveRoom();

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void BroadcastActiveRoomChanged();
};
