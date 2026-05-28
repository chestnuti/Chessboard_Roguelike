#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid/GridTypes.h"
#include "GridManager.generated.h"

class UGridSettings;
class UInstancedStaticMeshComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnTileTypeChanged,
	FIntPoint,
	TileCoord,
	ETileType,
	NewTileType);

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGridManager : public AActor
{
	GENERATED_BODY()

public:
	AGridManager();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TObjectPtr<UGridSettings> GridSettings;

	// One component renders all tile instances; TileData remains the source of gameplay truth.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	TObjectPtr<UInstancedStaticMeshComponent> TileISM;

	// Central grid state store keyed by logical coordinate.
	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	TMap<FIntPoint, FTileData> Tiles;

	// Visual layers listen here to update only the changed tile after Construct/Acid consumption.
	UPROPERTY(BlueprintAssignable, Category = "Grid|Events")
	FOnTileTypeChanged OnTileTypeChanged;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void GenerateGrid();

	// Coordinate and occupancy helpers are intentionally collision/NavMesh free.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	bool IsValidCoord(FIntPoint Coord) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	bool IsWalkable(FIntPoint Coord) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	bool IsOccupied(FIntPoint Coord) const;

	// Returns a copy so callers can inspect tile state without mutating the map.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	bool GetTileData(const FIntPoint& Coord, FTileData& OutTileData) const;

	// Changes terrain type only; occupancy, coordinates, and walkability are preserved.
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool SetTileType(const FIntPoint& Coord, ETileType NewTileType);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Grid|Visual")
	void RefreshTileInstanceVisual(const FIntPoint& Coord, ETileType NewTileType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid|Visual")
	bool GetTileInstanceIndex(const FIntPoint& Coord, int32& OutInstanceIndex) const;

	// Used by tile effects to avoid consuming obstacles or explicitly locked tiles.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	bool IsTileConvertible(const FIntPoint& Coord) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	FVector GridToWorld(FIntPoint Coord) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	FIntPoint WorldToGrid(FVector WorldLocation) const;

	// Attempts initial placement without modifying any other tile.
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool TryOccupyTile(FIntPoint Coord, AActor* Occupant, EGridOccupantType OccupantType);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void ClearOccupant(FIntPoint Coord);

	// Atomic logical move: all validation must pass before source/target occupancy changes.
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool RequestMove(AActor* Unit, FIntPoint FromCoord, FIntPoint ToCoord);

private:
	void ApplyTileInstanceCustomData(const FIntPoint& Coord, ETileType NewTileType);

	UPROPERTY(VisibleAnywhere, Category = "Grid")
	TObjectPtr<USceneComponent> SceneRoot;

	// Maps a grid coord to its instanced mesh index so one tile can be recolored without rebuilding the grid.
	UPROPERTY(Transient)
	TMap<FIntPoint, int32> TileInstanceIndices;
};
