#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/RTSGameInstance.h"
#include "RTSRoomEntryWidget.generated.h"

class URTSLobbyBrowserWidget;

UCLASS(Abstract, Blueprintable)
class MIMIC_STARCRAFT_API URTSRoomEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    FRTSRoomInfo RoomInfo;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby")
    TObjectPtr<URTSLobbyBrowserWidget> OwnerLobbyWidget;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RTS Lobby|Fail")
    TSubclassOf<UUserWidget> RoomEntryFailWidgetClass;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Lobby|Fail")
    TObjectPtr<UUserWidget> ActiveFailWidget;

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void SetupRoomEntry(const FRTSRoomInfo& InRoomInfo, URTSLobbyBrowserWidget* InOwner);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void UpdateRoomEntry(const FRTSRoomInfo& InRoomInfo);

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    void JoinThisRoom();

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool TryJoinThisRoom();
 
    UFUNCTION(BlueprintCallable, Category = "RTS Lobby")
    bool CanJoinRoom() const;

    UFUNCTION(BlueprintCallable, Category = "RTS Lobby|Fail")
    void ShowJoinFailWidget();

    UFUNCTION(BlueprintImplementableEvent, Category = "RTS Lobby")
    void OnRoomInfoUpdated(const FRTSRoomInfo& InRoomInfo);

    UFUNCTION(BlueprintImplementableEvent, Category = "RTS Lobby")
    void OnJoinSucceeded(const FRTSRoomInfo& InRoomInfo);

    UFUNCTION(BlueprintImplementableEvent, Category = "RTS Lobby")
    void OnJoinFailed(const FRTSRoomInfo& InRoomInfo);
};
