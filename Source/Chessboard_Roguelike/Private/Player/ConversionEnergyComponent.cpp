#include "Player/ConversionEnergyComponent.h"

UConversionEnergyComponent::UConversionEnergyComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UConversionEnergyComponent::HasConversionEnergy() const
{
	return bHasConversionEnergy;
}

ETileType UConversionEnergyComponent::GetHeldConversionEnergyType() const
{
	return HeldConversionEnergyType;
}

void UConversionEnergyComponent::GrantConversionEnergy(ETileType NewEnergyType)
{
	if (!IsGrantableEnergyType(NewEnergyType))
	{
		return;
	}

	const bool bWasHoldingEnergy = bHasConversionEnergy;
	const ETileType PreviousEnergyType = HeldConversionEnergyType;

	bHasConversionEnergy = true;
	HeldConversionEnergyType = NewEnergyType;

	OnConversionEnergyGranted.Broadcast(HeldConversionEnergyType);
	if (!bWasHoldingEnergy || PreviousEnergyType != HeldConversionEnergyType)
	{
		OnConversionEnergyChanged.Broadcast(bHasConversionEnergy, HeldConversionEnergyType);
	}
}

bool UConversionEnergyComponent::ConsumeConversionEnergy()
{
	if (!bHasConversionEnergy)
	{
		return false;
	}

	const ETileType ConsumedEnergyType = HeldConversionEnergyType;
	bHasConversionEnergy = false;
	HeldConversionEnergyType = ETileType::Minimal;

	OnConversionEnergyConsumed.Broadcast(ConsumedEnergyType);
	OnConversionEnergyChanged.Broadcast(bHasConversionEnergy, HeldConversionEnergyType);
	return true;
}

void UConversionEnergyComponent::ClearConversionEnergy()
{
	if (!bHasConversionEnergy && HeldConversionEnergyType == ETileType::Minimal)
	{
		return;
	}

	bHasConversionEnergy = false;
	HeldConversionEnergyType = ETileType::Minimal;
	OnConversionEnergyChanged.Broadcast(bHasConversionEnergy, HeldConversionEnergyType);
}

bool UConversionEnergyComponent::IsGrantableEnergyType(ETileType EnergyType) const
{
	return EnergyType == ETileType::Construct || EnergyType == ETileType::Acid;
}
