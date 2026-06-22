#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Transform/ChessTransformTypes.h"
#include "GridPlayerController.generated.h"

class AGridPawn;
class UChessPieceFormData;
class UInputAction;
class UInputMappingContext;
class UCombatCameraDirectorComponent;
class UMaterialParameterCollection;
class UPauseMenuWidget;
class UPlayerAttributeHUDWidget;
class UTutorialFlowComponent;
class UTransformWheelWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransformWheelOpened);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTransformSelectedForTutorial, UChessPieceFormData*, FormData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPauseMenuRequestReceived);

UENUM(BlueprintType)
enum class EPlayerControlMode : uint8
{
	DefaultWASD = 0 UMETA(DisplayName = "Default WASD"),
	TransformWheel = 1 UMETA(DisplayName = "Transform Wheel"),
	TransformTargeting = 2 UMETA(DisplayName = "Transform Targeting")
};

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGridPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGridPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatCameraDirectorComponent> CombatCameraDirectorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTutorialFlowComponent> TutorialFlowComponent;

	UFUNCTION(BlueprintCallable, Category = "Combat Camera")
	void FocusCombatCameraOnGridTile(const FVector& TargetWorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Conversion Energy Camera")
	void BeginConversionEnergyCameraZoom();

	UFUNCTION(BlueprintCallable, Category = "Conversion Energy Camera")
	void EndConversionEnergyCameraZoom();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Conversion Energy Camera")
	bool CanStartConversionEnergyCameraZoom() const;

	UFUNCTION(BlueprintCallable, Category = "Transform")
	void RequestSelectTransform(UChessPieceFormData* FormData);

	UFUNCTION(BlueprintCallable, Category = "Transform")
	void CancelTransformTargeting();

	UFUNCTION(BlueprintCallable, Category = "Transform|Camera")
	void NotifyTransformCameraDragStarted();

	UFUNCTION(BlueprintCallable, Category = "Transform|Camera")
	void NotifyTransformCameraDragEnded();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Transform")
	EPlayerControlMode GetPlayerControlMode() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Transform")
	TArray<FTransformMoveTarget> GetPendingTransformTargets() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Transform")
	TArray<UChessPieceFormData*> GetTransformWheelForms() const;

	UFUNCTION(BlueprintCallable, Category = "UI|Pause")
	void ShowPauseMenu();

	UFUNCTION(BlueprintCallable, Category = "UI|Pause")
	void ClosePauseMenu();

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnTransformWheelOpened OnTransformWheelOpened;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnTransformSelectedForTutorial OnTransformSelectedForTutorial;

	UPROPERTY(BlueprintAssignable, Category = "UI|Pause|Events")
	FOnPauseMenuRequestReceived OnPauseResumeRequested;

	UPROPERTY(BlueprintAssignable, Category = "UI|Pause|Events")
	FOnPauseMenuRequestReceived OnPauseBackRequested;

	UPROPERTY(BlueprintAssignable, Category = "UI|Pause|Events")
	FOnPauseMenuRequestReceived OnPauseSettingsRequested;

	UPROPERTY(BlueprintAssignable, Category = "UI|Pause|Events")
	FOnPauseMenuRequestReceived OnPauseMainMenuRequested;

	UPROPERTY(BlueprintAssignable, Category = "UI|Pause|Events")
	FOnPauseMenuRequestReceived OnPauseQuitGameRequested;

protected:
	// Mapping context and actions are assigned in Blueprint/Data assets so bindings stay data-driven.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> GridMovementMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveUpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveDownAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveLeftAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveRightAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> UseEnergyAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SwitchEnergyTypeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Transform")
	TObjectPtr<UInputAction> OpenTransformWheelAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Transform")
	TObjectPtr<UInputAction> TransformLeftClickAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Transform")
	TObjectPtr<UInputAction> TransformCancelAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Transform")
	TObjectPtr<UInputAction> TransformRightMouseAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UPlayerAttributeHUDWidget> PlayerAttributeHUDClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Pause")
	TSubclassOf<UPauseMenuWidget> PauseMenuClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Transform")
	TSubclassOf<UTransformWheelWidget> TransformWheelWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform")
	TArray<TObjectPtr<UChessPieceFormData>> TransformWheelForms;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Camera", meta = (ClampMin = "1.0"))
	float EdgeScrollZoneSize = 128.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Camera", meta = (ClampMin = "0.0"))
	float EdgeScrollSpeed = 1200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Camera", meta = (ClampMin = "0.0"))
	float EdgeScrollGridPaddingTiles = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Camera", meta = (ClampMin = "0.0"))
	float RightMouseDragCancelThreshold = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Material")
	TObjectPtr<UMaterialParameterCollection> MouseHoverGridParameterCollection;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Material")
	FName MouseHoverGridXParameterName = TEXT("MouseHoverGridX");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Material")
	FName MouseHoverGridYParameterName = TEXT("MouseHoverGridY");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Material")
	FName HasMouseHoverGridParameterName = TEXT("bHasMouseHoverGrid");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Material")
	bool bWriteMouseHoverGridOnlyWhileTransformTargeting = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Material")
	bool bMouseHoverRequiresPendingTransformTarget = true;

	// Runtime HUD instance owned by the local controller.
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UPlayerAttributeHUDWidget> PlayerAttributeHUD;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Pause")
	TObjectPtr<UPauseMenuWidget> PauseMenuWidget;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Transform")
	TObjectPtr<UTransformWheelWidget> TransformWheelWidget;

private:
	void MoveUp();
	void MoveDown();
	void MoveLeft();
	void MoveRight();
	void RequestPawnMove(FIntPoint Direction);
	void HandleUseEnergyStarted();
	void HandleUseEnergyFinished();
	void HandleSwitchEnergyType();
	void HandleTransformWheelStarted();
	void HandleTransformWheelReleased();
	void HandleTransformLeftClick();
	void HandleTransformCancel();
	void HandleTransformRightMouseStarted();
	void HandleTransformRightMouseReleased();
	UFUNCTION()
	void HandlePauseResumeRequested();
	UFUNCTION()
	void HandlePauseBackRequested();
	UFUNCTION()
	void HandlePauseSettingsRequested();
	UFUNCTION()
	void HandlePauseMainMenuRequested();
	UFUNCTION()
	void HandlePauseQuitGameRequested();
	void ShowTransformWheel();
	void HideTransformWheel();
	void RefreshTransformWheel();
	void FinishTransformTargeting();
	void ReturnToDefaultWASD();
	void ShowTransformTargetHighlights();
	void ClearTransformTargetHighlights();
	void UpdateEdgeScroll(float DeltaTime);
	void UpdatePendingTransformCameraRestore();
	void ApplyCameraEdgeScroll(const FVector2D& ScreenDirection, float DeltaTime);
	void UpdateMouseHoverGridMaterialParameters();
	void WriteMouseHoverGridMaterialParameters(const FIntPoint& HoverCoord, bool bHasHover);
	UMaterialParameterCollection* GetMouseHoverGridParameterCollection() const;
	bool TryGetGridCoordUnderMouse(FIntPoint& OutGridCoord) const;
	bool HasPendingTransformTarget(FIntPoint Coord) const;
	AGridPawn* GetGridPawn() const;

	UPROPERTY(Transient)
	TObjectPtr<UChessPieceFormData> PendingTransformForm;

	UPROPERTY(Transient)
	TArray<FTransformMoveTarget> PendingTransformTargets;

	EPlayerControlMode ControlMode = EPlayerControlMode::DefaultWASD;
	bool bEdgeScrollEnabled = false;
	bool bIsCameraDragging = false;
	bool bTransformSelectionInProgress = false;
	bool bRestoreCameraAfterTransformMove = false;
	bool bRightMousePressedForTransform = false;
	bool bHasWrittenMouseHoverGridParameters = false;
	bool bLastWrittenMouseHoverGridHasHover = false;
	FIntPoint LastWrittenMouseHoverGridCoord = FIntPoint::ZeroValue;
	FVector2D TransformRightMouseDownPosition = FVector2D::ZeroVector;
};
