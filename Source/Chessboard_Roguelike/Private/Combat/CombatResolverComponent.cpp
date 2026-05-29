#include "Combat/CombatResolverComponent.h"

#include "Enemy/GridEnemyPawn.h"
#include "Player/PlayerAttributeComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatResolver, Log, All);

UCombatResolverComponent::UCombatResolverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FCombatDamage UCombatResolverComponent::BuildPlayerDamage(const AActor* PlayerActor) const
{
	FCombatDamage Damage;
	if (!PlayerActor)
	{
		UE_LOG(LogCombatResolver, Warning, TEXT("BuildPlayerDamage failed: PlayerActor is null."));
		return Damage;
	}

	const UPlayerAttributeComponent* AttributeComponent = PlayerActor->FindComponentByClass<UPlayerAttributeComponent>();
	if (!AttributeComponent)
	{
		UE_LOG(LogCombatResolver, Warning, TEXT("BuildPlayerDamage failed: %s has no PlayerAttributeComponent."),
			*GetNameSafe(PlayerActor));
		return Damage;
	}

	Damage.ConstructDamage = AttributeComponent->GetConstructValue();
	Damage.AcidDamage = AttributeComponent->GetAcidValue();
	return Damage;
}

FCombatResolveResult UCombatResolverComponent::ResolvePlayerMeleeAttack(const AActor* PlayerActor, const AGridEnemyPawn* EnemyActor) const
{
	FCombatResolveResult Result;
	if (!PlayerActor || !EnemyActor)
	{
		UE_LOG(LogCombatResolver, Warning, TEXT("ResolvePlayerMeleeAttack failed: PlayerActor or EnemyActor is null."));
		return Result;
	}

	if (!EnemyActor->CanReceiveDamage())
	{
		return Result;
	}

	const FCombatDamage Damage = BuildPlayerDamage(PlayerActor);
	Result.KillThreshold = FMath::Max(1, EnemyActor->KillThreshold);

	Result.bImmuneConstruct = EnemyActor->Faction == EEnemyFaction::Construct;
	Result.bImmuneAcid = EnemyActor->Faction == EEnemyFaction::Acid;
	Result.EffectiveConstructDamage = Result.bImmuneConstruct ? 0 : Damage.ConstructDamage;
	Result.EffectiveAcidDamage = Result.bImmuneAcid ? 0 : Damage.AcidDamage;

	const int32 EffectiveSingleHitDamage = FMath::Max(Result.EffectiveConstructDamage, Result.EffectiveAcidDamage);
	Result.bKilled = EffectiveSingleHitDamage >= Result.KillThreshold;
	return Result;
}
