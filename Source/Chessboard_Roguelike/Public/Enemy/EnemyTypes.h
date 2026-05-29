#pragma once

#include "CoreMinimal.h"
#include "EnemyTypes.generated.h"

UENUM(BlueprintType)
enum class EEnemyFaction : uint8
{
	Construct UMETA(DisplayName = "Construct"),
	Acid UMETA(DisplayName = "Acid")
};

UENUM(BlueprintType)
enum class EEnemyBehaviorType : uint8
{
	Melee UMETA(DisplayName = "Melee"),
	Ranged UMETA(DisplayName = "Ranged")
};
