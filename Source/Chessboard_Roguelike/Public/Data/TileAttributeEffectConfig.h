#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Grid/GridTypes.h"
#include "TileAttributeEffectConfig.generated.h"

// Optional tuning asset for task 4 tile-enter attribute deltas and tile consumption behavior.
UCLASS(BlueprintType)
class CHESSBOARD_ROGUELIKE_API UTileAttributeEffectConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// Construct tile effect: raises Construct and usually lowers Acid.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Attribute Effects")
	int32 ConstructTile_ConstructDelta = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Attribute Effects")
	int32 ConstructTile_AcidDelta = -1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Attribute Effects")
	int32 AcidTile_ConstructDelta = -1;

	// Attribute values are clamped after deltas are applied.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Attribute Effects")
	int32 AcidTile_AcidDelta = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Attribute Effects")
	int32 MinAttributeValue = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Attribute Effects")
	int32 MaxAttributeValue = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Attribute Effects")
	bool bConsumeTileAfterEnter = true;

	// Result tile type after a Construct/Acid tile is consumed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tile Attribute Effects")
	ETileType ConsumedTileResultType = ETileType::Minimal;
};
