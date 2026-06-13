// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/RTSRoomEntryWidget.h"
#include "UI/RTSLobbyBrowserWidget.h"

void URTSRoomEntryWidget::SetupRoomEntry(const FRTSRoomInfo& InRoomInfo, URTSLobbyBrowserWidget* InOwner)
{
    OwnerLobbyWidget = InOwner;
    UpdateRoomEntry(InRoomInfo);
}

void URTSRoomEntryWidget::UpdateRoomEntry(const FRTSRoomInfo& InRoomInfo)
{
    RoomInfo = InRoomInfo;
    OnRoomInfoUpdated(RoomInfo);
}

void URTSRoomEntryWidget::JoinThisRoom()
{
    TryJoinThisRoom();
}

bool URTSRoomEntryWidget::TryJoinThisRoom()
{
    if (!CanJoinRoom())
    {
        ShowJoinFailWidget();
        OnJoinFailed(RoomInfo);
        return false;
    }

    if (OwnerLobbyWidget)
    {
        const bool bJoinedRoom = OwnerLobbyWidget->JoinRoom(RoomInfo.RoomId);
        if (bJoinedRoom)
        {
            OnJoinSucceeded(RoomInfo);
            return true;
        }

        ShowJoinFailWidget();
        OnJoinFailed(RoomInfo);
        return false;
    }

    ShowJoinFailWidget();
    OnJoinFailed(RoomInfo);
    return false;
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
