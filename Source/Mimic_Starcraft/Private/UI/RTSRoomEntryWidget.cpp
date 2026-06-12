// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/RTSRoomEntryWidget.h"
#include "UI/RTSLobbyBrowserWidget.h"

void URTSRoomEntryWidget::SetupRoomEntry(const FRTSRoomInfo& InRoomInfo, URTSLobbyBrowserWidget* InOwner)
{
    RoomInfo = InRoomInfo;
    OwnerLobbyWidget = InOwner;
    OnRoomInfoUpdated(RoomInfo);
}

void URTSRoomEntryWidget::JoinThisRoom()
{
    if (!CanJoinRoom())
    {
        ShowJoinFailWidget();
    }

    if (OwnerLobbyWidget)
    {
        OwnerLobbyWidget->JoinRoom(RoomInfo.RoomId);
    }

}

bool URTSRoomEntryWidget::CanJoinRoom() const
{
    return RoomInfo.bIsJoinable && RoomInfo.CurrentPlayers < RoomInfo.MaxPlayers;
}

void URTSRoomEntryWidget::ShowJoinFailWidget()
{
    if (!RoomEntryFailWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("RoomEntryFailWidgetClass is not set."));
        return;
    }

    APlayerController* OwningPlayer = GetOwningPlayer();

    UUserWidget* FailWidget = CreateWidget<UUserWidget>(OwningPlayer, RoomEntryFailWidgetClass);
    if (!FailWidget)
    {
        return;
    }

    ActiveFailWidget = FailWidget;
    ActiveFailWidget->AddToViewport(100);
}
