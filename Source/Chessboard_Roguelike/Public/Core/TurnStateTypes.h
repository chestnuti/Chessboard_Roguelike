#pragma once

#include "CoreMinimal.h"
#include "TurnStateTypes.generated.h"

UENUM(BlueprintType)
enum class ETurnState : uint8
{
	// Setup phase before the grid, pawn placement, and managers are ready.
	Initializing,
	// The only state in which player input can start an action.
	PlayerInput,
	// Short lock window while movement/effects resolve.
	PlayerActionResolve,
	// Reserved for later enemy logic; intentionally unused by the current prototype.
	EnemyTurnResolve,
	// Reserved for later map/tile resolution after actor actions.
	MapResolve
};
