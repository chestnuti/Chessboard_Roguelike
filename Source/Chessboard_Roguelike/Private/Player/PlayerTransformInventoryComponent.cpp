#include "Player/PlayerTransformInventoryComponent.h"

UPlayerTransformInventoryComponent::UPlayerTransformInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPlayerTransformInventoryComponent::AddTransformPiece(EChessTransformType TransformType, int32 Amount)
{
	if (TransformType == EChessTransformType::None || Amount <= 0)
	{
		return false;
	}

	int32& Count = TransformStacks.FindOrAdd(TransformType);
	Count += Amount;
	BroadcastInventoryChanged(TransformType);
	return true;
}

bool UPlayerTransformInventoryComponent::ConsumeTransformPiece(EChessTransformType TransformType, int32 Amount)
{
	if (!CanConsumeTransformPiece(TransformType, Amount))
	{
		return false;
	}

	int32& Count = TransformStacks.FindChecked(TransformType);
	Count -= Amount;
	if (Count <= 0)
	{
		TransformStacks.Remove(TransformType);
	}

	BroadcastInventoryChanged(TransformType);
	return true;
}

int32 UPlayerTransformInventoryComponent::GetTransformPieceCount(EChessTransformType TransformType) const
{
	if (const int32* Count = TransformStacks.Find(TransformType))
	{
		return *Count;
	}

	return 0;
}

bool UPlayerTransformInventoryComponent::CanConsumeTransformPiece(EChessTransformType TransformType, int32 Amount) const
{
	if (TransformType == EChessTransformType::None || Amount <= 0)
	{
		return false;
	}

	return GetTransformPieceCount(TransformType) >= Amount;
}

TMap<EChessTransformType, int32> UPlayerTransformInventoryComponent::GetTransformStacks() const
{
	return TransformStacks;
}

void UPlayerTransformInventoryComponent::BroadcastInventoryChanged(EChessTransformType TransformType)
{
	OnTransformInventoryChanged.Broadcast();
	OnTransformInventorySlotChanged.Broadcast(TransformType, GetTransformPieceCount(TransformType));
}
