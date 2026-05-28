#pragma once

#include "CoreMinimal.h"
#include "GridTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class ETileType : uint8
{
	Minimal,
	Construct,
	Acid,
	Obstacle
};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint GridCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETileType TileType = ETileType::Minimal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridOccupantType OccupantType = EGridOccupantType::Empty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<AActor> OccupantActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bWalkable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bConvertible = true;
};
