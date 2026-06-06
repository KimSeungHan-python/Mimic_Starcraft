#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RTSGameInstance.generated.h"

USTRUCT(BlueprintType)
struct MIMIC_STARCRAFT_API FRTSRoomInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    FName RoomId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    FText RoomName;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    FText HostName;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    FName MapName = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    int32 CurrentPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    int32 MaxPlayers = 2;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Room")
    bool bIsJoinable = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRTSRoomListChanged);

UCLASS()
class MIMIC_STARCRAFT_API URTSGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|Maps")
    FName MainMenuMapName = TEXT("MainMenu");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|Maps")
    FName LobbyBrowserMapName = TEXT("LobbyBrowser");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|Maps")
    FName LobbyMapName = TEXT("Lobby");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|Maps")
    FName GameMapName = TEXT("GPTTestMap");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Flow|Rooms")
    int32 DefaultMaxPlayers = 2;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Flow|Rooms")
    TArray<FRTSRoomInfo> CachedRooms;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Flow|Rooms")
    FRTSRoomInfo ActiveRoom;

    UPROPERTY(BlueprintAssignable, Category = "RTS Flow|Rooms")
    FRTSRoomListChanged OnRoomListChanged;

    UFUNCTION(BlueprintCallable, Category = "RTS Flow")
    void OpenMainMenu();

    UFUNCTION(BlueprintCallable, Category = "RTS Flow")
    void OpenLobbyBrowser();

    UFUNCTION(BlueprintCallable, Category = "RTS Flow")
    void OpenLobby();

    UFUNCTION(BlueprintCallable, Category = "RTS Flow")
    void StartGameFromLobby();

    UFUNCTION(BlueprintCallable, Category = "RTS Flow|Rooms")
    void RefreshLocalRoomList();

    UFUNCTION(BlueprintCallable, Category = "RTS Flow|Rooms")
    bool HostLocalRoom(const FText& RoomName, const FText& HostName);

    UFUNCTION(BlueprintCallable, Category = "RTS Flow|Rooms")
    bool JoinLocalRoom(FName RoomId);

protected:
    void OpenConfiguredLevel(FName MapName) const;
};
