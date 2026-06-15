#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Transform/ChessTransformTypes.h"
#include "PlayerTransformInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTransformInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnTransformInventorySlotChanged,
	EChessTransformType,
	TransformType,
	int32,
	NewCount);

UCLASS(ClassGroup=(Custom), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHESSBOARD_ROGUELIKE_API UPlayerTransformInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerTransformInventoryComponent();

	UPROPERTY(BlueprintAssignable, Category = "Transform Inventory|Events")
	FOnTransformInventoryChanged OnTransformInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Transform Inventory|Events")
	FOnTransformInventorySlotChanged OnTransformInventorySlotChanged;

	UFUNCTION(BlueprintCallable, Category = "Transform Inventory")
	bool AddTransformPiece(EChessTransformType TransformType, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, Category = "Transform Inventory")
	bool ConsumeTransformPiece(EChessTransformType TransformType, int32 Amount = 1);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Transform Inventory")
	int32 GetTransformPieceCount(EChessTransformType TransformType) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Transform Inventory")
	bool CanConsumeTransformPiece(EChessTransformType TransformType, int32 Amount = 1) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Transform Inventory")
	TMap<EChessTransformType, int32> GetTransformStacks() const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transform Inventory", meta = (AllowPrivateAccess = "true"))
	TMap<EChessTransformType, int32> TransformStacks;

	void BroadcastInventoryChanged(EChessTransformType TransformType);
};
