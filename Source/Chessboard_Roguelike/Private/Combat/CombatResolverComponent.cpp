#include "Combat/CombatResolverComponent.h"

#include "Enemy/GridEnemyPawn.h"
#include "Player/GridPawn.h"
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

FEnemyAttackDamage UCombatResolverComponent::BuildEnemyMeleeDamage(const AGridEnemyPawn* EnemyActor) const
{
	FEnemyAttackDamage Damage;
	if (!EnemyActor || !EnemyActor->IsAlive())
	{
		UE_LOG(LogCombatResolver, Warning, TEXT("BuildEnemyMeleeDamage failed: EnemyActor is null or dead."));
		return Damage;
	}

	Damage.HealthDamage = FMath::Max(0, EnemyActor->AttackDamage);

	if (EnemyActor->bApplyFactionAttributeDamage && EnemyActor->AttackAttributeDamage > 0)
	{
		switch (EnemyActor->Faction)
		{
		case EEnemyFaction::Construct:
			Damage.ConstructDelta = -EnemyActor->AttackAttributeDamage;
			break;
		case EEnemyFaction::Acid:
			Damage.AcidDelta = -EnemyActor->AttackAttributeDamage;
			break;
		default:
			break;
		}
	}

	return Damage;
}

FEnemyAttackResolveResult UCombatResolverComponent::ResolveEnemyMeleeAttack(
	const AGridEnemyPawn* EnemyActor,
	AGridPawn* PlayerPawn) const
{
	FEnemyAttackResolveResult Result;
	if (!EnemyActor || !PlayerPawn)
	{
		UE_LOG(LogCombatResolver, Warning, TEXT("ResolveEnemyMeleeAttack failed: EnemyActor or PlayerPawn is null."));
		return Result;
	}

	UPlayerAttributeComponent* AttributeComponent = PlayerPawn->PlayerAttributeComponent;
	if (!AttributeComponent)
	{
		AttributeComponent = PlayerPawn->FindComponentByClass<UPlayerAttributeComponent>();
	}

	if (!AttributeComponent || AttributeComponent->IsDefeated())
	{
		return Result;
	}

	const FEnemyAttackDamage Damage = BuildEnemyMeleeDamage(EnemyActor);
	if (Damage.ConstructDelta != 0 || Damage.AcidDelta != 0)
	{
		AttributeComponent->ApplyTileAttributeDelta(Damage.ConstructDelta, Damage.AcidDelta);
		Result.AppliedConstructDelta = Damage.ConstructDelta;
		Result.AppliedAcidDelta = Damage.AcidDelta;
	}

	if (Damage.HealthDamage > 0)
	{
		AttributeComponent->ApplyHealthDamage(Damage.HealthDamage);
		Result.AppliedHealthDamage = Damage.HealthDamage;
	}

	Result.RemainingHealth = AttributeComponent->GetCurrentHealth();
	Result.bPlayerDefeated = AttributeComponent->IsDefeated();
	Result.bDamageApplied = Result.AppliedHealthDamage > 0 || Result.AppliedConstructDelta != 0 || Result.AppliedAcidDelta != 0;
	return Result;
}

FEnemyFriendlyFireResolveResult UCombatResolverComponent::ResolveEnemyRangedFriendlyFire(
	const AGridEnemyPawn* Attacker,
	const AGridEnemyPawn* TargetEnemy)
{
	FEnemyFriendlyFireResolveResult Result;
	if (!Attacker || !TargetEnemy)
	{
		UE_LOG(LogCombatResolver, Warning, TEXT("ResolveEnemyRangedFriendlyFire failed: Attacker or TargetEnemy is null."));
		return Result;
	}

	if (!Attacker->IsAlive() || !TargetEnemy->IsAlive())
	{
		return Result;
	}

	Result.bResolved = true;
	if (Attacker->Faction == TargetEnemy->Faction)
	{
		Result.bSameFaction = true;
		Result.Reason = EEnemyFriendlyFireResolveReason::SameFaction;
		return Result;
	}

	Result.bTargetKilled = true;
	Result.Reason = EEnemyFriendlyFireResolveReason::RangedDifferentFaction;
	return Result;
}

FEnemyFriendlyFireResolveResult UCombatResolverComponent::ResolveEnemyMeleeCollision(
	const AGridEnemyPawn* Attacker,
	const AGridEnemyPawn* TargetEnemy,
	ETileType CollisionTileType)
{
	FEnemyFriendlyFireResolveResult Result;
	Result.CollisionTileType = CollisionTileType;

	if (!Attacker || !TargetEnemy)
	{
		UE_LOG(LogCombatResolver, Warning, TEXT("ResolveEnemyMeleeCollision failed: Attacker or TargetEnemy is null."));
		return Result;
	}

	if (!Attacker->IsAlive() || !TargetEnemy->IsAlive())
	{
		return Result;
	}

	Result.bResolved = true;
	if (Attacker->Faction == TargetEnemy->Faction)
	{
		Result.bSameFaction = true;
		Result.Reason = EEnemyFriendlyFireResolveReason::SameFaction;
		return Result;
	}

	switch (CollisionTileType)
	{
	case ETileType::Construct:
		Result.bAttackerKilled = Attacker->Faction == EEnemyFaction::Acid;
		Result.bTargetKilled = TargetEnemy->Faction == EEnemyFaction::Acid;
		Result.Reason = EEnemyFriendlyFireResolveReason::ConstructTileKillsAcid;
		break;

	case ETileType::Acid:
		Result.bAttackerKilled = Attacker->Faction == EEnemyFaction::Construct;
		Result.bTargetKilled = TargetEnemy->Faction == EEnemyFaction::Construct;
		Result.Reason = EEnemyFriendlyFireResolveReason::AcidTileKillsConstruct;
		break;

	case ETileType::Minimal:
	default:
	{
		const int32 AttackerThreshold = FMath::Max(1, Attacker->KillThreshold);
		const int32 TargetThreshold = FMath::Max(1, TargetEnemy->KillThreshold);
		if (AttackerThreshold < TargetThreshold)
		{
			Result.bAttackerKilled = true;
			Result.Reason = EEnemyFriendlyFireResolveReason::MinimalTileLowerThreshold;
		}
		else if (TargetThreshold < AttackerThreshold)
		{
			Result.bTargetKilled = true;
			Result.Reason = EEnemyFriendlyFireResolveReason::MinimalTileLowerThreshold;
		}
		else
		{
			Result.bAttackerKilled = true;
			Result.bTargetKilled = true;
			Result.Reason = EEnemyFriendlyFireResolveReason::MinimalTileEqualThreshold;
		}
		break;
	}
	}

	return Result;
}
