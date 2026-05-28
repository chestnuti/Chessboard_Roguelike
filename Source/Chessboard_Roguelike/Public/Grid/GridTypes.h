#pragma once

#include "CoreMinimal.h"
#include "GridTypes.generated.h"

class AActor;

// Logical terrain categories used by grid rules and lightweight Blueprint debugging.
UENUM(BlueprintType)
enum class ETileType : uint8
{
	// Explicit values preserve existing serialized assets while adding Blueprint display names.
	Minimal = 0 UMETA(DisplayName = "Minimal"),
	Construct = 1 UMETA(DisplayName = "Construct"),
	Acid = 2 UMETA(DisplayName = "Acid"),
	Obstacle = 3 UMETA(DisplayName = "Obstacle")
};

// Occupancy is tracked separately from physical collision so movement remains deterministic.
UENUM(BlueprintType)
enum class EGridOccupantType : uint8
{
	Empty,
	Player,
	Enemy,
	Obstacle
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FTileData
{
	GENERATED_BODY()

	// Stable logical address; actor world location is derived from this coordinate.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint GridCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETileType TileType = ETileType::Minimal;

	// OccupantType is the fast gameplay query; OccupantActor keeps the exact object reference.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridOccupantType OccupantType = EGridOccupantType::Empty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<AActor> OccupantActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bWalkable = true;

	// Future tile effects may consume or transform only tiles that opt in here.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bConvertible = true;
};
