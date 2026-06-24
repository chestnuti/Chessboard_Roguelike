#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid/GridTypes.h"
#include "GridManager.generated.h"

class UGridSettings;
class UGridObstacleVisualComponent;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid|Obstacle")
	TObjectPtr<UInstancedStaticMeshComponent> ObstacleCubeISM;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid|Obstacle")
	TObjectPtr<UInstancedStaticMeshComponent> ObstacleFaceISM;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid|Obstacle")
	TObjectPtr<UGridObstacleVisualComponent> ObstacleVisualComponent;

	// Central grid state store keyed by logical coordinate.
	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	TMap<FIntPoint, FTileData> Tiles;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	int32 CurrentWidth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	int32 CurrentHeight = 0;

	// Visual layers listen here to update only the changed tile after Construct/Acid consumption.
	UPROPERTY(BlueprintAssignable, Category = "Grid|Events")
	FOnTileTypeChanged OnTileTypeChanged;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void GenerateGrid();

	// Applies a complete generated layout. Procedural systems should use this instead of mutating Tiles directly.
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool ApplyTileLayout(const TArray<FGridTileLayoutData>& TileLayout, int32 LayoutWidth, int32 LayoutHeight);

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

	// Changes terrain and synchronizes blocking state for obstacle creation/removal.
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool SetTileBlockingType(const FIntPoint& Coord, ETileType NewTileType);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Grid|Visual")
	void RefreshTileInstanceVisual(const FIntPoint& Coord, ETileType NewTileType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid|Visual")
	bool GetTileInstanceIndex(const FIntPoint& Coord, int32& OutInstanceIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Grid|Visual")
	void ClearPlayerNextMoveTiles();

	UFUNCTION(BlueprintCallable, Category = "Grid|Visual")
	void SetPlayerNextMoveTiles(const TArray<FIntPoint>& MoveCoords);

	// Used by tile effects to avoid consuming obstacles or explicitly locked tiles.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	bool IsTileConvertible(const FIntPoint& Coord) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	FVector GridToWorld(FIntPoint Coord) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	FIntPoint WorldToGrid(FVector WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Grid|Pathfinding")
	bool FindPathAStar(FIntPoint StartCoord, FIntPoint GoalCoord, TArray<FIntPoint>& OutPath, bool bAllowOccupiedGoal = false) const;

	// Attempts initial placement without modifying any other tile.
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool TryOccupyTile(FIntPoint Coord, AActor* Occupant, EGridOccupantType OccupantType);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void ClearOccupant(FIntPoint Coord);

	// Atomic logical move: all validation must pass before source/target occupancy changes.
	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool RequestMove(AActor* Unit, FIntPoint FromCoord, FIntPoint ToCoord);

private:
	bool HasValidGridSettings() const;
	void ClearGridState();
	void BuildDefaultTileLayout(TArray<FGridTileLayoutData>& OutTileLayout) const;
	void ApplyInitialTileOverrides(TArray<FGridTileLayoutData>& TileLayout) const;
	void ApplyTileLayoutInternal(const TArray<FGridTileLayoutData>& TileLayout);
	FTileData MakeTileDataFromLayout(const FGridTileLayoutData& LayoutData) const;
	void RebuildTileVisuals();
	void AddTileVisualInstance(const FTileData& TileData);
	void ApplyTileInstanceCustomData(const FIntPoint& Coord, ETileType NewTileType);
	void SetTilePlayerNextMoveCustomData(const FIntPoint& Coord, bool bCanMoveNext);

	UPROPERTY(VisibleAnywhere, Category = "Grid")
	TObjectPtr<USceneComponent> SceneRoot;

	// Maps a grid coord to its instanced mesh index so one tile can be recolored without rebuilding the grid.
	UPROPERTY(Transient)
	TMap<FIntPoint, int32> TileInstanceIndices;

	UPROPERTY(Transient)
	TSet<FIntPoint> PlayerNextMoveTileCoords;
};
