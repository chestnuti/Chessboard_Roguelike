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
	// Enemy manager is resolving active enemy actions.
	EnemyTurnResolve,
	// Reserved for later map/tile resolution after actor actions.
	MapResolve,
	// Final validation point for win/loss logic after actors and map effects resolve.
	CheckEndCondition,
	// Terminal state after all active enemies or objectives are cleared.
	Victory,
	// Terminal state after the player's HP reaches zero.
	Defeat
};
