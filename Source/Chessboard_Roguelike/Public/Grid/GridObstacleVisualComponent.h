#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Grid/GridTypes.h"
#include "Grid/ObstacleVisualConfig.h"
#include "GridObstacleVisualComponent.generated.h"

class AGridManager;
class UMaterialInstanceDynamic;
class UObstacleVisualConfig;
class UStaticMesh;

UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class CHESSBOARD_ROGUELIKE_API UGridObstacleVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridObstacleVisualComponent();

	UFUNCTION(BlueprintCallable, Category = "Grid|Obstacle")
	void RebuildFromGrid(AGridManager* GridManager);

	UFUNCTION(BlueprintCallable, Category = "Grid|Obstacle")
	void RefreshObstacleAtCoord(AGridManager* GridManager, FIntPoint Coord);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid|Obstacle")
	bool GetCubeInstanceIndex(const FIntPoint& Coord, int32& OutInstanceIndex) const;

protected:
	virtual void OnRegister() override;

private:
	void ClearObstacleVisuals(AGridManager* GridManager);
	void ConfigureComponents(AGridManager* GridManager);
	void ApplyWaveMaterialParameters();
	void AddObstacleCube(const AGridManager* GridManager, const FTileData& TileData);
	void AddObstacleFaces(const AGridManager* GridManager, const FTileData& TileData);
	void AddFaceInstance(const AGridManager* GridManager, const FTileData& TileData, const FIntPoint& Direction, EObstacleFaceVisualStyle Style);
	bool HasNearbyWalkableTile(const AGridManager* GridManager, const FIntPoint& Coord) const;
	bool GetTileTypeAt(const AGridManager* GridManager, const FIntPoint& Coord, ETileType& OutTileType) const;
	EObstacleFaceVisualStyle GetFaceStyleForNeighbor(ETileType NeighborType) const;
	FVector GetMeshScaleForDesiredSize(const UStaticMesh* Mesh, const FVector& DesiredSize) const;
	float GetStablePhase(const FIntPoint& Coord) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Obstacle", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UObstacleVisualConfig> VisualConfig;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CubeMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FaceMaterialInstance;

	UPROPERTY(Transient)
	TMap<FIntPoint, int32> CubeInstanceIndices;
};
