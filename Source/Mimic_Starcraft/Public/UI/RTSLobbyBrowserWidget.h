#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/RTSGameInstance.h"
#include "RTSLobbyBrowserWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class MIMIC_STARCRAFT_API URTSLobbyBrowserWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    TObjectPtr<URTSGameInstance> RTSGameInstance;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    TArray<FRTSRoomInfo> Rooms;

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void BindToGameInstance(URTSGameInstance* InGameInstance);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void RefreshRooms();

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool HostRoom(const FText& RoomName, const FText& HostName);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool JoinRoom(FName RoomId);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void OpenMainMenu();

    UFUNCTION(BlueprintImplementableEvent, Category = "RTS Lobby")
    void OnRoomsUpdated(const TArray<FRTSRoomInfo>& UpdatedRooms);
};
