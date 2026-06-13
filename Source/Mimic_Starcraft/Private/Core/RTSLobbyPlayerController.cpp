#include "Core/RTSLobbyPlayerController.h"

#include "Core/RTSGameInstance.h"
#include "Core/RTSLobbyGameMode.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"

ARTSLobbyPlayerController::ARTSLobbyPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void ARTSLobbyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController())
    {
        bShowMouseCursor = true;
        bEnableClickEvents = true;
        bEnableMouseOverEvents = true;

        FInputModeUIOnly InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
    }
}

void ARTSLobbyPlayerController::RequestLeaveLobby()
{
    if (HasAuthority())
    {
        ServerLeaveLobby_Implementation();
        return;
    }

    ServerLeaveLobby();
}

void ARTSLobbyPlayerController::ClientRequestLobbyRegistration_Implementation()
{
    URTSGameInstance* RTSGameInstance = GetGameInstance<URTSGameInstance>();
    if (!RTSGameInstance)
    {
        return;
    }

    const FName PlayerId = RTSGameInstance->EnsureLocalLobbyPlayerId();
    const FText PlayerName = RTSGameInstance->LocalLobbyPlayerName.IsEmpty()
        ? FText::FromString(TEXT("Player"))
        : RTSGameInstance->LocalLobbyPlayerName;

    ServerSubmitLobbyPlayer(
        PlayerId,
        PlayerName,
        RTSGameInstance->SelectedTeamNumber,
        RTSGameInstance->SelectedRace,
        RTSGameInstance->SelectedPlayerColor
    );
}

void ARTSLobbyPlayerController::ClientReturnToLobbyBrowser_Implementation()
{
    if (IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get())
    {
        IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
        if (SessionInterface && SessionInterface->GetNamedSession(FName(TEXT("RTSGameSession"))))
        {
            SessionInterface->DestroySession(FName(TEXT("RTSGameSession")));
        }
    }

    if (URTSGameInstance* RTSGameInstance = GetGameInstance<URTSGameInstance>())
    {
        RTSGameInstance->OpenLobbyBrowser();
    }
}

void ARTSLobbyPlayerController::ServerSubmitLobbyPlayer_Implementation(
    FName PlayerId,
    const FText& PlayerName,
    int32 TeamNumber,
    ERTSRace Race,
    FLinearColor PlayerColor
)
{
    if (ARTSLobbyGameMode* LobbyGameMode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ARTSLobbyGameMode>()
        : nullptr)
    {
        LobbyGameMode->RegisterLobbyPlayer(this, PlayerId, PlayerName, TeamNumber, Race, PlayerColor);
    }
}

void ARTSLobbyPlayerController::ServerSetLobbyPlayerTeam_Implementation(FName TargetPlayerId, int32 TeamNumber)
{
    if (ARTSLobbyGameMode* LobbyGameMode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ARTSLobbyGameMode>()
        : nullptr)
    {
        LobbyGameMode->SetLobbyPlayerTeam(this, TargetPlayerId, TeamNumber);
    }
}

void ARTSLobbyPlayerController::ServerSetLobbyPlayerName_Implementation(FName TargetPlayerId, const FText& PlayerName)
{
    if (ARTSLobbyGameMode* LobbyGameMode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ARTSLobbyGameMode>()
        : nullptr)
    {
        LobbyGameMode->SetLobbyPlayerName(this, TargetPlayerId, PlayerName);
    }
}

void ARTSLobbyPlayerController::ServerSetLobbyPlayerRace_Implementation(FName TargetPlayerId, ERTSRace Race)
{
    if (ARTSLobbyGameMode* LobbyGameMode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ARTSLobbyGameMode>()
        : nullptr)
    {
        LobbyGameMode->SetLobbyPlayerRace(this, TargetPlayerId, Race);
    }
}

void ARTSLobbyPlayerController::ServerSetLobbyPlayerColor_Implementation(FName TargetPlayerId, FLinearColor PlayerColor)
{
    if (ARTSLobbyGameMode* LobbyGameMode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ARTSLobbyGameMode>()
        : nullptr)
    {
        LobbyGameMode->SetLobbyPlayerColor(this, TargetPlayerId, PlayerColor);
    }
}

void ARTSLobbyPlayerController::ServerStartLobbyGame_Implementation()
{
    if (ARTSLobbyGameMode* LobbyGameMode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ARTSLobbyGameMode>()
        : nullptr)
    {
        LobbyGameMode->StartLobbyGame(this);
    }
}

void ARTSLobbyPlayerController::ServerLeaveLobby_Implementation()
{
    if (ARTSLobbyGameMode* LobbyGameMode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ARTSLobbyGameMode>()
        : nullptr)
    {
        LobbyGameMode->LeaveLobby(this);
    }
}
