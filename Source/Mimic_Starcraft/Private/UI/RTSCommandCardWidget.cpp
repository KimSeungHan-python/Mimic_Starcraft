#include "UI/RTSCommandCardWidget.h"

#include "Core/RTSPlayerController.h"

void URTSCommandCardWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BindToPlayerController(GetOwningPlayer<ARTSPlayerController>());
}

void URTSCommandCardWidget::NativeDestruct()
{
    if (RTSPlayerController)
    {
        RTSPlayerController->OnSelectionChanged.RemoveDynamic(this, &URTSCommandCardWidget::RefreshCommands);
    }

    Super::NativeDestruct();
}

void URTSCommandCardWidget::BindToPlayerController(ARTSPlayerController* InPlayerController)
{
    if (RTSPlayerController == InPlayerController)
    {
        RefreshCommands();
        return;
    }

    if (RTSPlayerController)
    {
        RTSPlayerController->OnSelectionChanged.RemoveDynamic(this, &URTSCommandCardWidget::RefreshCommands);
    }

    RTSPlayerController = InPlayerController;

    if (RTSPlayerController)
    {
        RTSPlayerController->OnSelectionChanged.AddUniqueDynamic(this, &URTSCommandCardWidget::RefreshCommands);
    }

    RefreshCommands();
}

void URTSCommandCardWidget::RefreshCommands()
{
    Commands.Reset();

    if (RTSPlayerController)
    {
        RTSPlayerController->GetAvailableCommandButtons(Commands);
    }

    OnCommandCardUpdated(Commands);
}

bool URTSCommandCardWidget::ExecuteCommandAtSlot(int32 SlotIndex)
{
    const bool bExecuted = RTSPlayerController
        ? RTSPlayerController->ExecuteCommandSlot(SlotIndex)
        : false;

    RefreshCommands();
    return bExecuted;
}

bool URTSCommandCardWidget::ExecuteCommand(const FRTSCommandButton& Command)
{
    const bool bExecuted = RTSPlayerController
        ? RTSPlayerController->ExecuteCommandButton(Command)
        : false;

    RefreshCommands();
    return bExecuted;
}

bool URTSCommandCardWidget::ExecuteCommandHotkey(FKey Hotkey)
{
    const bool bExecuted = RTSPlayerController
        ? RTSPlayerController->ExecuteCommandHotkey(Hotkey)
        : false;

    RefreshCommands();
    return bExecuted;
}

bool URTSCommandCardWidget::GetCommandAtSlot(int32 SlotIndex, FRTSCommandButton& OutCommand) const
{
    for (const FRTSCommandButton& Command : Commands)
    {
        if (Command.SlotIndex == SlotIndex)
        {
            OutCommand = Command;
            return true;
        }
    }

    OutCommand = FRTSCommandButton();
    return false;
}

bool URTSCommandCardWidget::HasCommandAtSlot(int32 SlotIndex) const
{
    FRTSCommandButton UnusedCommand;
    return GetCommandAtSlot(SlotIndex, UnusedCommand);
}

FText URTSCommandCardWidget::GetCommandHotkeyDisplayText(const FRTSCommandButton& Command) const
{
    return Command.Hotkey.IsValid()
        ? Command.Hotkey.GetDisplayName()
        : FText::GetEmpty();
}
