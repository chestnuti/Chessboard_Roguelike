#pragma once

#include "CoreMinimal.h"
#include "TurnStateTypes.generated.h"

UENUM(BlueprintType)
enum class ETurnState : uint8
{
	Initializing,
	PlayerInput,
	PlayerActionResolve,
	EnemyTurnResolve,
	MapResolve
};
