#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TransformWheelWidget.generated.h"

class AGridPlayerController;
class UChessPieceFormData;
class UButton;
class UTextBlock;
class UPlayerTransformInventoryComponent;

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API UTransformWheelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Transform Wheel")
	void InitializeWheel(AGridPlayerController* InOwningGridController);

	UFUNCTION(BlueprintCallable, Category = "Transform Wheel")
	void RefreshFromInventory(UPlayerTransformInventoryComponent* InventoryComponent);

	UFUNCTION(BlueprintCallable, Category = "Transform Wheel")
	void RequestSelectTransform(UChessPieceFormData* FormData);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Transform Wheel")
	AGridPlayerController* GetOwningGridController() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Transform Wheel")
	void OnWheelInitialized();

	UFUNCTION(BlueprintImplementableEvent, Category = "Transform Wheel")
	void OnInventoryRefreshed(UPlayerTransformInventoryComponent* InventoryComponent);

private:
	UPROPERTY(Transient)
	TObjectPtr<AGridPlayerController> OwningGridController;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerTransformInventoryComponent> CachedInventoryComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UChessPieceFormData>> SlotForms;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> SlotButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SlotLabels;

	UFUNCTION()
	void HandleSlot0Clicked();

	UFUNCTION()
	void HandleSlot1Clicked();

	UFUNCTION()
	void HandleSlot2Clicked();

	UFUNCTION()
	void HandleSlot3Clicked();

	void BuildDefaultWheelIfNeeded();
	void HandleSlotClicked(int32 SlotIndex);
	void RefreshSlotLabels();
};
