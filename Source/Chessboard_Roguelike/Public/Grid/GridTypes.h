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

// Layout role describes how a tile participates in the map shape; TileType remains the resource/terrain effect.
UENUM(BlueprintType)
enum class EGridCellRole : uint8
{
	Open = 0 UMETA(DisplayName = "Open"),
	Room = 1 UMETA(DisplayName = "Room"),
	Corridor = 2 UMETA(DisplayName = "Corridor"),
	Wall = 3 UMETA(DisplayName = "Wall"),
	Start = 4 UMETA(DisplayName = "Start"),
	Exit = 5 UMETA(DisplayName = "Exit")
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridCellRole CellRole = EGridCellRole::Open;

	// Generated rooms/regions can stamp this for activation, difficulty, and debugging.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RegionId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Depth = 0;

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

// Lightweight layout payload consumed by GridManager; procedural systems should output this, not mutate Tiles directly.
USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FGridTileLayoutData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	FIntPoint GridCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	ETileType TileType = ETileType::Minimal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	EGridCellRole CellRole = EGridCellRole::Open;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bWalkable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bConvertible = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 RegionId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 Depth = 0;
};
