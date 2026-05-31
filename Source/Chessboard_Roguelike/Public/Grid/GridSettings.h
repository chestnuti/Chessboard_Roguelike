#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Grid/GridTypes.h"
#include "GridSettings.generated.h"

class UStaticMesh;

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FGridInitialTileOverride
{
	GENERATED_BODY()

	// Authored coordinate override used for prototype test layouts.
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

UCLASS(BlueprintType)
class CHESSBOARD_ROGUELIKE_API UGridSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	// Logical grid dimensions; GenerateGrid creates Width * Height FTileData entries.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 Width = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 Height = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	float TileSize = 200.f;

	// World-space anchor for GridCoord(0,0).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	FVector Origin = FVector::ZeroVector;

	// Optional visual mesh for InstancedStaticMesh rendering; gameplay does not depend on it.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TObjectPtr<UStaticMesh> TileMesh;

	// Manual prototype overrides for authored test tiles; this is not procedural generation.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TArray<FGridInitialTileOverride> InitialTileOverrides;
};
