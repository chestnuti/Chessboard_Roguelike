#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Grid/GridTypes.h"
#include "TileEffectResolverComponent.generated.h"

class AGridManager;
class UTileAttributeEffectConfig;

// Resolves only tile-enter effects after a confirmed move; combat and AI are intentionally outside this component.
UCLASS(ClassGroup=(Grid), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHESSBOARD_ROGUELIKE_API UTileEffectResolverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTileEffectResolverComponent();

	// Called by the pawn after a successful logical move onto TileCoord.
	UFUNCTION(BlueprintCallable, Category = "Tile Effects")
	void ResolveTileEnterEffect(AActor* PlayerActor, const FIntPoint& TileCoord);

	// Explicit wiring avoids world searches during normal pawn initialization.
	UFUNCTION(BlueprintCallable, Category = "Tile Effects")
	void SetGridManager(AGridManager* InGridManager);

protected:
	// Optional tuning asset; defaults are used when this is not assigned.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tile Effects")
	TObjectPtr<UTileAttributeEffectConfig> TileAttributeEffectConfig;

	UPROPERTY(BlueprintReadOnly, Category = "Tile Effects")
	TObjectPtr<AGridManager> GridManager;

private:
	AGridManager* ResolveGridManager();
	bool ShouldConsumeEnteredTile() const;
	ETileType GetConsumedTileResultType() const;
};
