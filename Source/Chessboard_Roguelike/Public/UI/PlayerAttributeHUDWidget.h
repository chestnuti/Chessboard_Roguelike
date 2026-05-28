#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerAttributeHUDWidget.generated.h"

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

	// Safe to call from events or Blueprint when the displayed values need a refresh.
	UFUNCTION(BlueprintCallable, Category = "Player Attributes")
	void RefreshAttributeDisplay();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Optional bindings match the Widget Blueprint names; native fallback widgets are created if they are absent.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UTextBlock> ConstructText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UTextBlock> AcidText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UProgressBar> ConstructProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UProgressBar> AcidProgressBar;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPlayerAttributeComponent> AttributeComponent;

	UFUNCTION()
	void HandlePlayerAttributeChanged(int32 NewConstructValue, int32 NewAcidValue);

	void BuildFallbackWidgetTreeIfNeeded();
	void BindToAttributeComponent(UPlayerAttributeComponent* InAttributeComponent);
	void UnbindFromAttributeComponent();
};
