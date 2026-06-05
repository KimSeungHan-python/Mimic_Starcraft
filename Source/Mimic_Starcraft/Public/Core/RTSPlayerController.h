#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Types/RTSGridTypes.h"
#include "RTSPlayerController.generated.h"

class ARTSGridManager;
class URTSBuildingData;
class ARTSBuilding;
class ARTSBuildGridPreview;

UCLASS()
class MIMIC_STARCRAFT_API ARTSPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ARTSPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building")
    ARTSGridManager* GridManager;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building")
    URTSBuildingData* SelectedBuildingData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building")
    TSubclassOf<ARTSBuilding> DefaultBuildingClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building")
    TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

    UPROPERTY(BlueprintReadOnly, Category = "RTS Building")
    bool bIsInBuildMode = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building|Preview")
    UMaterialInterface* ValidPreviewMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building|Preview")
    UMaterialInterface* InvalidPreviewMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Building|Preview")
    TSubclassOf<ARTSBuildGridPreview> BuildGridPreviewClass;

private:
    UPROPERTY()
    ARTSBuilding* PreviewBuildingActor = nullptr;

    UPROPERTY()
    FTransform LastPreviewTransform;

    UPROPERTY()
    bool bHasValidPreviewTransform = false;

    FRTSGridCoord CurrentPreviewCoord;
    bool bCurrentPlacementValid = false;

    UPROPERTY()
    ARTSBuildGridPreview* BuildGridPreviewActor = nullptr;

public:
    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void StartBuildMode(URTSBuildingData* BuildingData);

    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void CancelBuildMode();

    UFUNCTION(BlueprintCallable, Category = "RTS Building")
    void ConfirmBuild();

    UFUNCTION(Client, Reliable)
    void Client_SetStartCamera(const FTransform& CameraTransform);

private:
    bool GetMouseWorldLocation(FVector& OutLocation) const;
    void UpdateBuildingPreview();
    void CreatePreviewActor();
    void DestroyPreviewActor();
    void SetPreviewValidVisual(bool bValid);
    void CreateBuildGridPreviewActor();
    void DestroyBuildGridPreviewActor();
};