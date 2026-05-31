#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FCombatDamage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 ConstructDamage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 AcidDamage = 0;
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FCombatResolveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bKilled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bImmuneConstruct = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bImmuneAcid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 EffectiveConstructDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 EffectiveAcidDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 KillThreshold = 0;
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FEnemyAttackDamage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 HealthDamage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 ConstructDelta = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	int32 AcidDelta = 0;
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FEnemyAttackResolveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bDamageApplied = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bPlayerDefeated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 AppliedHealthDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 AppliedConstructDelta = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 AppliedAcidDelta = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	int32 RemainingHealth = 0;
};
