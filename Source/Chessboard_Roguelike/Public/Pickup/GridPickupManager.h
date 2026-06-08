#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid/GridTypes.h"
#include "GridPickupManager.generated.h"

class AGridPickupActor;
class AGridPawn;
class USceneComponent;

// Runtime registry for pickups keyed by grid coordinate.
UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGridPickupManager : public AActor
{
	GENERATED_BODY()

public:
	AGridPickupManager();

	UPROPERTY(BlueprintReadOnly, Category = "Pickup")
	TMap<FIntPoint, TObjectPtr<AGridPickupActor>> PickupsByCoord;

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	bool RegisterPickup(AGridPickupActor* Pickup);

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void UnregisterPickupAt(FIntPoint Coord);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pickup")
	AGridPickupActor* GetPickupAt(FIntPoint Coord) const;

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	bool TryCollectPickupAt(FIntPoint Coord, AGridPawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void ClearAllPickups();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;
};
