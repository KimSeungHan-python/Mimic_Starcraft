#include "UI/RTSSelectionPanelWidget.h"

#include "Buildings/RTSBuilding.h"
#include "Components/RTSHealthComponent.h"
#include "Core/RTSPlayerController.h"
#include "Data/RTSBuildingData.h"
#include "Data/RTSUnitData.h"
#include "Units/RTSUnitBase.h"

void URTSSelectionPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BindToPlayerController(GetOwningPlayer<ARTSPlayerController>());
}

void URTSSelectionPanelWidget::NativeDestruct()
{
    if (RTSPlayerController)
    {
        RTSPlayerController->OnSelectionChanged.RemoveDynamic(this, &URTSSelectionPanelWidget::RefreshSelectionItems);
    }

    ClearHealthBindings();

    Super::NativeDestruct();
}

void URTSSelectionPanelWidget::BindToPlayerController(ARTSPlayerController* InPlayerController)
{
    if (RTSPlayerController == InPlayerController)
    {
        RefreshSelectionItems();
        return;
    }

    if (RTSPlayerController)
    {
        RTSPlayerController->OnSelectionChanged.RemoveDynamic(this, &URTSSelectionPanelWidget::RefreshSelectionItems);
    }

    RTSPlayerController = InPlayerController;

    if (RTSPlayerController)
    {
        RTSPlayerController->OnSelectionChanged.AddUniqueDynamic(this, &URTSSelectionPanelWidget::RefreshSelectionItems);
    }

    RefreshSelectionItems();
}

void URTSSelectionPanelWidget::RefreshSelectionItems()
{
    ClearHealthBindings();
    SelectionItems.Reset();

    if (!RTSPlayerController)
    {
        OnSelectionPanelUpdated(SelectionItems);
        return;
    }

    for (AActor* SelectedActor : RTSPlayerController->GetSelectedActors())
    {
        if (!SelectedActor)
        {
            continue;
        }

        if (!SelectedActor->IsA<ARTSUnitBase>() && !SelectedActor->IsA<ARTSBuilding>())
        {
            continue;
        }

        SelectionItems.Add(BuildSelectionItem(SelectedActor));
        BindHealthComponent(SelectedActor->FindComponentByClass<URTSHealthComponent>());
    }

    OnSelectionPanelUpdated(SelectionItems);
}

bool URTSSelectionPanelWidget::GetSelectionItemAtIndex(int32 Index, FRTSSelectionPanelItem& OutItem) const
{
    if (!SelectionItems.IsValidIndex(Index))
    {
        OutItem = FRTSSelectionPanelItem();
        return false;
    }

    OutItem = SelectionItems[Index];
    return true;
}

ERTSSelectionHealthLevel URTSSelectionPanelWidget::GetHealthLevelForFraction(float HealthFraction) const
{
    if (HealthFraction >= HighHealthThreshold)
    {
        return ERTSSelectionHealthLevel::High;
    }

    if (HealthFraction >= MediumHealthThreshold)
    {
        return ERTSSelectionHealthLevel::Medium;
    }

    return ERTSSelectionHealthLevel::Low;
}

FLinearColor URTSSelectionPanelWidget::GetHealthTintForLevel(ERTSSelectionHealthLevel HealthLevel) const
{
    switch (HealthLevel)
    {
    case ERTSSelectionHealthLevel::High:
        return HighHealthTint;

    case ERTSSelectionHealthLevel::Medium:
        return MediumHealthTint;

    case ERTSSelectionHealthLevel::Low:
        return LowHealthTint;

    default:
        return FLinearColor::White;
    }
}

void URTSSelectionPanelWidget::HandleBoundHealthChanged(URTSHealthComponent* HealthComponent, float CurrentHealth, float MaxHealth)
{
    RefreshSelectionItems();
}

void URTSSelectionPanelWidget::ClearHealthBindings()
{
    for (URTSHealthComponent* HealthComponent : BoundHealthComponents)
    {
        if (HealthComponent)
        {
            HealthComponent->OnHealthChanged.RemoveDynamic(this, &URTSSelectionPanelWidget::HandleBoundHealthChanged);
        }
    }

    BoundHealthComponents.Reset();
}

void URTSSelectionPanelWidget::BindHealthComponent(URTSHealthComponent* HealthComponent)
{
    if (!HealthComponent || BoundHealthComponents.Contains(HealthComponent))
    {
        return;
    }

    HealthComponent->OnHealthChanged.AddUniqueDynamic(this, &URTSSelectionPanelWidget::HandleBoundHealthChanged);
    BoundHealthComponents.Add(HealthComponent);
}

FRTSSelectionPanelItem URTSSelectionPanelWidget::BuildSelectionItem(AActor* Actor) const
{
    FRTSSelectionPanelItem Item;
    Item.Actor = Actor;

    if (const ARTSUnitBase* Unit = Cast<ARTSUnitBase>(Actor))
    {
        Item.bIsUnit = true;

        if (Unit->UnitData)
        {
            Item.DisplayName = Unit->UnitData->DisplayName.IsEmpty()
                ? FText::FromName(Unit->UnitData->UnitId)
                : Unit->UnitData->DisplayName;
            Item.Icon = Unit->UnitData->SelectionIcon
                ? Unit->UnitData->SelectionIcon
                : Unit->UnitData->CommandIcon;
        }
    }
    else if (const ARTSBuilding* Building = Cast<ARTSBuilding>(Actor))
    {
        Item.bIsBuilding = true;

        if (Building->BuildingData)
        {
            Item.DisplayName = Building->BuildingData->DisplayName.IsEmpty()
                ? FText::FromName(Building->BuildingData->BuildingId)
                : Building->BuildingData->DisplayName;
            Item.Icon = Building->BuildingData->SelectionIcon
                ? Building->BuildingData->SelectionIcon
                : Building->BuildingData->CommandIcon;
        }
    }

    if (Item.DisplayName.IsEmpty() && Actor)
    {
        Item.DisplayName = FText::FromString(Actor->GetName());
    }

    if (const URTSHealthComponent* HealthComponent = Actor ? Actor->FindComponentByClass<URTSHealthComponent>() : nullptr)
    {
        Item.HealthFraction = HealthComponent->GetHealthFraction();
        Item.HealthLevel = GetHealthLevelForFraction(Item.HealthFraction);
        Item.HealthTint = GetHealthTintForLevel(Item.HealthLevel);
    }

    return Item;
}
