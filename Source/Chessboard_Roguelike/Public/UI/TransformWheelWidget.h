#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TransformWheelWidget.generated.h"

class AGridPlayerController;
class UChessPieceFormData;
class UButton;
class UImage;
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
	TArray<TObjectPtr<UImage>> SlotIcons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SlotLabels;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SlotCountLabels;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> Slot_Knight;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> Slot_Bishop;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> Slot_Rook;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> Slot_Queen;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UImage> Icon_Knight;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UImage> Icon_Bishop;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UImage> Icon_Rook;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UImage> Icon_Queen;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> NameText_Knight;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> NameText_Bishop;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> NameText_Rook;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> NameText_Queen;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> CountText_Knight;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> CountText_Bishop;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> CountText_Rook;

	UPROPERTY(BlueprintReadOnly, Category = "Transform Wheel|Bound Widgets", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> CountText_Queen;

	UFUNCTION()
	void HandleSlot0Clicked();

	UFUNCTION()
	void HandleSlot1Clicked();

	UFUNCTION()
	void HandleSlot2Clicked();

	UFUNCTION()
	void HandleSlot3Clicked();

	void BuildDefaultWheelIfNeeded();
	void CacheDesignedSlotWidgets();
	void BindSlotButtonDelegates();
	void HandleSlotClicked(int32 SlotIndex);
	void RefreshSlotLabels();
};
