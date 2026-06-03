#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"
#include "Components/ActorComponent.h"
#include "CombatResolverComponent.generated.h"

class AGridEnemyPawn;
class AGridPawn;

UCLASS(ClassGroup=(Combat), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHESSBOARD_ROGUELIKE_API UCombatResolverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatResolverComponent();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	FCombatDamage BuildPlayerDamage(const AActor* PlayerActor) const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FCombatResolveResult ResolvePlayerMeleeAttack(const AActor* PlayerActor, const AGridEnemyPawn* EnemyActor) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Combat")
	FEnemyAttackDamage BuildEnemyMeleeDamage(const AGridEnemyPawn* EnemyActor) const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FEnemyAttackResolveResult ResolveEnemyMeleeAttack(const AGridEnemyPawn* EnemyActor, AGridPawn* PlayerPawn) const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Friendly Fire")
	static FEnemyFriendlyFireResolveResult ResolveEnemyRangedFriendlyFire(
		const AGridEnemyPawn* Attacker,
		const AGridEnemyPawn* TargetEnemy);

	UFUNCTION(BlueprintCallable, Category = "Combat|Friendly Fire")
	static FEnemyFriendlyFireResolveResult ResolveEnemyMeleeCollision(
		const AGridEnemyPawn* Attacker,
		const AGridEnemyPawn* TargetEnemy,
		ETileType CollisionTileType);
};
