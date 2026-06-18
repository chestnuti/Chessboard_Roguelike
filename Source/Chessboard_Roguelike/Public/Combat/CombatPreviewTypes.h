#pragma once

#include "CoreMinimal.h"
#include "CombatPreviewTypes.generated.h"

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FCombatPreviewState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Combat Preview")
	bool bCanBeKilledByPlayer = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Preview")
	bool bCanAttackPreviewPlayerCoord = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat Preview")
	bool bIsRelevant = false;
};
