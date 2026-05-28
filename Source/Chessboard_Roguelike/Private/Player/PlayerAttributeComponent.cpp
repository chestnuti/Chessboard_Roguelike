#include "Player/PlayerAttributeComponent.h"

UPlayerAttributeComponent::UPlayerAttributeComponent()
{
	// Attribute updates are event-driven; HUD refreshes through OnPlayerAttributeChanged instead of Tick.
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerAttributeComponent::ApplyTileAttributeDelta(int32 ConstructDelta, int32 AcidDelta)
{
	const int32 OldConstructValue = ConstructValue;
	const int32 OldAcidValue = AcidValue;

	ConstructValue = FMath::Clamp(ConstructValue + ConstructDelta, 0, MaxConstructValue);
	AcidValue = FMath::Clamp(AcidValue + AcidDelta, 0, MaxAcidValue);

	// Broadcast only when the clamped result actually changes, avoiding redundant HUD refreshes.
	if (ConstructValue != OldConstructValue || AcidValue != OldAcidValue)
	{
		OnPlayerAttributeChanged.Broadcast(ConstructValue, AcidValue);
	}
}

void UPlayerAttributeComponent::AddConstructValue(int32 Delta)
{
	ApplyTileAttributeDelta(Delta, 0);
}

void UPlayerAttributeComponent::AddAcidValue(int32 Delta)
{
	ApplyTileAttributeDelta(0, Delta);
}

int32 UPlayerAttributeComponent::GetConstructValue() const
{
	return ConstructValue;
}

int32 UPlayerAttributeComponent::GetAcidValue() const
{
	return AcidValue;
}

int32 UPlayerAttributeComponent::GetMaxConstructValue() const
{
	return MaxConstructValue;
}

int32 UPlayerAttributeComponent::GetMaxAcidValue() const
{
	return MaxAcidValue;
}

float UPlayerAttributeComponent::GetConstructRatio() const
{
	return MaxConstructValue > 0 ? static_cast<float>(ConstructValue) / static_cast<float>(MaxConstructValue) : 0.f;
}

float UPlayerAttributeComponent::GetAcidRatio() const
{
	return MaxAcidValue > 0 ? static_cast<float>(AcidValue) / static_cast<float>(MaxAcidValue) : 0.f;
}

bool UPlayerAttributeComponent::IsConstructValueMaxed() const
{
	return ConstructValue >= MaxConstructValue;
}

bool UPlayerAttributeComponent::IsAcidValueMaxed() const
{
	return AcidValue >= MaxAcidValue;
}
