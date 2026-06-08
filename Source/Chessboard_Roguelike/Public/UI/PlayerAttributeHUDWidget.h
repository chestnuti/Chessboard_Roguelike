#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Grid/GridTypes.h"
#include "PlayerAttributeHUDWidget.generated.h"

class UConversionEnergyComponent;
class UPlayerAttributeComponent;
class UProgressBar;
class UTextBlock;

// Read-only attribute HUD that refreshes from UPlayerAttributeComponent events instead of Tick.
UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API UPlayerAttributeHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Allows the controller to bind the HUD before NativeConstruct discovers the owning pawn.
	UFUNCTION(BlueprintCallable, Category = "Player Attributes")
	void InitializeFromAttributeComponent(UPlayerAttributeComponent* InAttributeComponent);

	// Allows the controller or Blueprint to bind the HUD to the player's conversion energy component.
	UFUNCTION(BlueprintCallable, Category = "Conversion Energy")
	void InitializeFromConversionEnergyComponent(UConversionEnergyComponent* InEnergyComponent);

	// Safe to call from events or Blueprint when the displayed values need a refresh.
	UFUNCTION(BlueprintCallable, Category = "Player Attributes")
	void RefreshAttributeDisplay();

	UFUNCTION(BlueprintCallable, Category = "Conversion Energy")
	void RefreshConversionEnergyDisplay();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Conversion Energy")
	FText GetConversionEnergyStatusText() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Optional bindings match the Widget Blueprint names; native fallback widgets are created if they are absent.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UTextBlock> ConstructText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UTextBlock> AcidText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UProgressBar> ConstructProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UProgressBar> AcidProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Conversion Energy")
	TObjectPtr<UTextBlock> EnergyText;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPlayerAttributeComponent> AttributeComponent;

	UPROPERTY(Transient)
	TObjectPtr<UConversionEnergyComponent> ConversionEnergyComponent;

	UFUNCTION()
	void HandlePlayerAttributeChanged(int32 NewConstructValue, int32 NewAcidValue);

	UFUNCTION()
	void HandlePlayerHealthChanged(int32 NewHealth, int32 MaxHealth);

	UFUNCTION()
	void HandleConversionEnergyChanged(bool bHasEnergy, ETileType EnergyType);

	void BuildFallbackWidgetTreeIfNeeded();
	void BindToAttributeComponent(UPlayerAttributeComponent* InAttributeComponent);
	void UnbindFromAttributeComponent();
	void BindToConversionEnergyComponent(UConversionEnergyComponent* InEnergyComponent);
	void UnbindFromConversionEnergyComponent();
};
