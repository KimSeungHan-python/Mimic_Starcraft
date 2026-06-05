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

    // 서버 RPC에서 BuildingId로 DataAsset을 다시 찾기 위한 목록
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS Building")
    TArray<URTSBuildingData*> BuildingDataList;

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

    ARTSGridManager* ResolveGridManager();

protected:
    UFUNCTION(Server, Reliable)
    void ServerConfirmBuild(FName BuildingId, FRTSGridCoord OriginCoord);

private:
    bool GetMouseWorldLocation(FVector& OutLocation) const;
    void UpdateBuildingPreview();
    void CreatePreviewActor();
    void DestroyPreviewActor();
    void SetPreviewValidVisual(bool bValid);
    void CreateBuildGridPreviewActor();
    void DestroyBuildGridPreviewActor();

    void BuildOnServer(FName BuildingId, FRTSGridCoord OriginCoord);
    URTSBuildingData* FindBuildingDataById(FName BuildingId) const;
};