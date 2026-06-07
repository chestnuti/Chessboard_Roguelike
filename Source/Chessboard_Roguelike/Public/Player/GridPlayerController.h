#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GridPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UCombatCameraDirectorComponent;
class UPlayerAttributeHUDWidget;

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGridPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGridPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatCameraDirectorComponent> CombatCameraDirectorComponent;

	UFUNCTION(BlueprintCallable, Category = "Combat Camera")
	void FocusCombatCameraOnGridTile(const FVector& TargetWorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Conversion Energy Camera")
	void BeginConversionEnergyCameraZoom();

	UFUNCTION(BlueprintCallable, Category = "Conversion Energy Camera")
	void EndConversionEnergyCameraZoom();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Conversion Energy Camera")
	bool CanStartConversionEnergyCameraZoom() const;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UPlayerAttributeHUDWidget> PlayerAttributeHUDClass;

	// Runtime HUD instance owned by the local controller.
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UPlayerAttributeHUDWidget> PlayerAttributeHUD;

private:
	void MoveUp();
	void MoveDown();
	void MoveLeft();
	void MoveRight();
	void RequestPawnMove(FIntPoint Direction);
	void HandleUseEnergyStarted();
	void HandleUseEnergyFinished();
};
