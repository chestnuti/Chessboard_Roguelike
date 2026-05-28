#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid/GridTypes.h"
#include "GridManager.generated.h"

class UGridSettings;
class UInstancedStaticMeshComponent;
class USceneComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	TObjectPtr<UInstancedStaticMeshComponent> TileISM;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	TMap<FIntPoint, FTileData> Tiles;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void GenerateGrid();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	bool IsValidCoord(FIntPoint Coord) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	bool IsWalkable(FIntPoint Coord) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	bool IsOccupied(FIntPoint Coord) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	FVector GridToWorld(FIntPoint Coord) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	FIntPoint WorldToGrid(FVector WorldLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool TryOccupyTile(FIntPoint Coord, AActor* Occupant, EGridOccupantType OccupantType);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void ClearOccupant(FIntPoint Coord);

	UFUNCTION(BlueprintCallable, Category = "Grid")
	bool RequestMove(AActor* Unit, FIntPoint FromCoord, FIntPoint ToCoord);

private:
	UPROPERTY(VisibleAnywhere, Category = "Grid")
	TObjectPtr<USceneComponent> SceneRoot;
};
