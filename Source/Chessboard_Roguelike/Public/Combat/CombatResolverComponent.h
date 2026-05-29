#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"
#include "Components/ActorComponent.h"
#include "CombatResolverComponent.generated.h"

class AGridEnemyPawn;

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
};
