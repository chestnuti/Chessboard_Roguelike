#include "Player/PlayerAttributeComponent.h"

#include "Audio/GameAudioSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UPlayerAttributeComponent::UPlayerAttributeComponent()
{
	// Attribute updates are event-driven; HUD refreshes through OnPlayerAttributeChanged instead of Tick.
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerAttributeComponent::ApplyTileAttributeDelta(int32 ConstructDelta, int32 AcidDelta)
{
	const int32 OldConstructValue = ConstructValue;
	const int32 OldAcidValue = AcidValue;
	const bool bWasConstructMaxed = IsConstructValueMaxed();
	const bool bWasAcidMaxed = IsAcidValueMaxed();

	ConstructValue = FMath::Clamp(ConstructValue + ConstructDelta, 0, MaxConstructValue);
	AcidValue = FMath::Clamp(AcidValue + AcidDelta, 0, MaxAcidValue);

	// Broadcast only when the clamped result actually changes, avoiding redundant HUD refreshes.
	if (ConstructValue != OldConstructValue || AcidValue != OldAcidValue)
	{
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
			{
				if (ConstructValue > OldConstructValue || AcidValue > OldAcidValue)
				{
					AudioSubsystem->PlayPlayerGainAttributeSFX();
				}

				if ((!bWasConstructMaxed && IsConstructValueMaxed()) || (!bWasAcidMaxed && IsAcidValueMaxed()))
				{
					AudioSubsystem->PlayPlayerAttributeFullSFX();
				}
			}
		}

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

bool UPlayerAttributeComponent::ApplyHealthDamage(int32 DamageAmount)
{
	if (DamageAmount <= 0 || IsDefeated())
	{
		return false;
	}

	const int32 OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0, MaxHealth);

	if (CurrentHealth != OldHealth)
	{
		OnPlayerHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	}

	if (OldHealth > 0 && CurrentHealth <= 0)
	{
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
			{
				AudioSubsystem->PlayPlayerDeathSFX();
			}
		}

		OnPlayerDefeated.Broadcast();
	}

	return CurrentHealth != OldHealth;
}

bool UPlayerAttributeComponent::Heal(int32 HealAmount)
{
	if (HealAmount <= 0)
	{
		return false;
	}

	const int32 OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0, MaxHealth);

	if (CurrentHealth != OldHealth)
	{
		OnPlayerHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	}

	return CurrentHealth != OldHealth;
}

int32 UPlayerAttributeComponent::GetConstructValue() const
{
	return ConstructValue;
}

int32 UPlayerAttributeComponent::GetAcidValue() const
{
	return AcidValue;
}

int32 UPlayerAttributeComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

int32 UPlayerAttributeComponent::GetMaxHealth() const
{
	return MaxHealth;
}

int32 UPlayerAttributeComponent::GetMaxConstructValue() const
{
	return MaxConstructValue;
}

int32 UPlayerAttributeComponent::GetMaxAcidValue() const
{
	return MaxAcidValue;
}

float UPlayerAttributeComponent::GetHealthRatio() const
{
	return MaxHealth > 0 ? static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth) : 0.f;
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

bool UPlayerAttributeComponent::IsConstructSuppressionActive() const
{
	return IsConstructValueMaxed();
}

bool UPlayerAttributeComponent::IsAcidSuppressionActive() const
{
	return IsAcidValueMaxed();
}

bool UPlayerAttributeComponent::CanSuppressFaction(EEnemyFaction EnemyFaction) const
{
	switch (EnemyFaction)
	{
	case EEnemyFaction::Construct:
		return IsConstructSuppressionActive();
	case EEnemyFaction::Acid:
		return IsAcidSuppressionActive();
	default:
		return false;
	}
}

bool UPlayerAttributeComponent::IsDefeated() const
{
	return CurrentHealth <= 0;
}
