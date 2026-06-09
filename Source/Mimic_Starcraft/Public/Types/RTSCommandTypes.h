#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "RTSCommandTypes.generated.h"

class AActor;
class UTexture2D;
class URTSBuildingData;
class URTSUnitData;

UENUM(BlueprintType)
enum class ERTSCommandType : uint8
{
    None,
    TrainUnit,
    BuildStructure,
    OpenCommandPage,
    Stop,
    Back
};

USTRUCT(BlueprintType)
struct MIMIC_STARCRAFT_API FRTSCommandButton
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    int32 SlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    FName CommandId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    FText TooltipText;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    ERTSCommandType CommandType = ERTSCommandType::None;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    FKey Hotkey;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    FName CommandPage = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    FName TargetCommandPage = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    int32 SortOrder = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    int32 MineralCost = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    int32 VespeneCost = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    int32 SupplyCost = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    bool bEnabled = true;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    FText DisabledReason;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    TObjectPtr<URTSUnitData> UnitData = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Command")
    TObjectPtr<URTSBuildingData> BuildingData = nullptr;
};

USTRUCT(BlueprintType)
struct MIMIC_STARCRAFT_API FRTSControlGroup
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RTS Control Group")
    TArray<TObjectPtr<AActor>> Actors;

    UPROPERTY()
    double LastRecallTimeSeconds = -1.0;
};
