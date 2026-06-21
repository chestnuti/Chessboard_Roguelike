#include "Player/ConversionEnergyComponent.h"

#include "Audio/GameAudioSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

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
	return bHasConversionEnergy ? HeldConversionEnergyType : ETileType::Minimal;
}

void UConversionEnergyComponent::GrantConversionEnergy(ETileType IgnoredEnergyType)
{
	(void)IgnoredEnergyType;

	const bool bWasHoldingEnergy = bHasConversionEnergy;

	if (!IsSelectableEnergyType(HeldConversionEnergyType))
	{
		HeldConversionEnergyType = ETileType::Construct;
	}

	bHasConversionEnergy = true;

	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
		{
			AudioSubsystem->PlayPlayerGainEnergySFX();
		}
	}

	OnConversionEnergyGranted.Broadcast(HeldConversionEnergyType);
	if (!bWasHoldingEnergy)
	{
		OnConversionEnergyChanged.Broadcast(bHasConversionEnergy, HeldConversionEnergyType);
	}
}

bool UConversionEnergyComponent::SetHeldConversionEnergyType(ETileType NewEnergyType)
{
	if (!bHasConversionEnergy || !IsSelectableEnergyType(NewEnergyType) || HeldConversionEnergyType == NewEnergyType)
	{
		return false;
	}

	HeldConversionEnergyType = NewEnergyType;

	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
		{
			AudioSubsystem->PlayPlayerSwitchEnergySFX();
		}
	}

	OnConversionEnergyChanged.Broadcast(bHasConversionEnergy, HeldConversionEnergyType);
	return true;
}

bool UConversionEnergyComponent::CycleHeldConversionEnergyType()
{
	if (!bHasConversionEnergy)
	{
		return false;
	}

	return SetHeldConversionEnergyType(GetNextSelectableEnergyType(HeldConversionEnergyType));
}

bool UConversionEnergyComponent::ConsumeConversionEnergy()
{
	if (!bHasConversionEnergy)
	{
		return false;
	}

	const ETileType ConsumedEnergyType = HeldConversionEnergyType;
	bHasConversionEnergy = false;

	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
		{
			AudioSubsystem->PlayPlayerUseEnergySFX();
		}
	}

	OnConversionEnergyConsumed.Broadcast(ConsumedEnergyType);
	OnConversionEnergyChanged.Broadcast(bHasConversionEnergy, ETileType::Minimal);
	return true;
}

void UConversionEnergyComponent::ClearConversionEnergy()
{
	if (!bHasConversionEnergy)
	{
		return;
	}

	bHasConversionEnergy = false;
	OnConversionEnergyChanged.Broadcast(bHasConversionEnergy, ETileType::Minimal);
}

bool UConversionEnergyComponent::IsSelectableEnergyType(ETileType EnergyType) const
{
	return EnergyType == ETileType::Construct || EnergyType == ETileType::Acid;
}

ETileType UConversionEnergyComponent::GetNextSelectableEnergyType(ETileType EnergyType) const
{
	switch (EnergyType)
	{
	case ETileType::Construct:
		return ETileType::Acid;
	case ETileType::Acid:
	default:
		return ETileType::Construct;
	}
}
