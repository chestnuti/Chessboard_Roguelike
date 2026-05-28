#include "Grid/TileEffectResolverComponent.h"

#include "Data/TileAttributeEffectConfig.h"
#include "EngineUtils.h"
#include "Grid/GridManager.h"
#include "Player/PlayerAttributeComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogTileEffectResolver, Log, All);

UTileEffectResolverComponent::UTileEffectResolverComponent()
{
	// Tile effects run once when movement succeeds; no per-frame work is needed.
	PrimaryComponentTick.bCanEverTick = false;
}

void UTileEffectResolverComponent::ResolveTileEnterEffect(AActor* PlayerActor, const FIntPoint& TileCoord)
{
	if (!PlayerActor)
	{
		UE_LOG(LogTileEffectResolver, Warning, TEXT("ResolveTileEnterEffect ignored: PlayerActor is null."));
		return;
	}

	AGridManager* ResolvedGridManager = ResolveGridManager();
	if (!ResolvedGridManager)
	{
		UE_LOG(LogTileEffectResolver, Warning, TEXT("ResolveTileEnterEffect ignored: GridManager not found."));
		return;
	}

	FTileData TileData;
	if (!ResolvedGridManager->GetTileData(TileCoord, TileData))
	{
		UE_LOG(LogTileEffectResolver, Warning, TEXT("ResolveTileEnterEffect ignored: invalid tile (%d,%d)."),
			TileCoord.X, TileCoord.Y);
		return;
	}

	if (TileData.OccupantType != EGridOccupantType::Player || TileData.OccupantActor.Get() != PlayerActor)
	{
		// Prevent enemy-occupied or stale tile data from accidentally granting player attributes.
		return;
	}

	UPlayerAttributeComponent* AttributeComponent = PlayerActor->FindComponentByClass<UPlayerAttributeComponent>();
	if (!AttributeComponent)
	{
		UE_LOG(LogTileEffectResolver, Warning, TEXT("ResolveTileEnterEffect ignored: %s has no PlayerAttributeComponent."),
			*GetNameSafe(PlayerActor));
		return;
	}

	switch (TileData.TileType)
	{
	case ETileType::Construct:
		// Construct tiles increase Construct and reduce Acid, then may be consumed below.
		AttributeComponent->ApplyTileAttributeDelta(
			TileAttributeEffectConfig ? TileAttributeEffectConfig->ConstructTile_ConstructDelta : 1,
			TileAttributeEffectConfig ? TileAttributeEffectConfig->ConstructTile_AcidDelta : -1);
		break;

	case ETileType::Acid:
		// Acid tiles increase Acid and reduce Construct, then may be consumed below.
		AttributeComponent->ApplyTileAttributeDelta(
			TileAttributeEffectConfig ? TileAttributeEffectConfig->AcidTile_ConstructDelta : -1,
			TileAttributeEffectConfig ? TileAttributeEffectConfig->AcidTile_AcidDelta : 1);
		break;

	case ETileType::Minimal:
	case ETileType::Obstacle:
	default:
		return;
	}

	if (ShouldConsumeEnteredTile() && ResolvedGridManager->IsTileConvertible(TileCoord))
	{
		// Consumption changes only the tile type; coordinates and occupancy stay intact.
		ResolvedGridManager->SetTileType(TileCoord, GetConsumedTileResultType());
	}
}

void UTileEffectResolverComponent::SetGridManager(AGridManager* InGridManager)
{
	GridManager = InGridManager;
}

AGridManager* UTileEffectResolverComponent::ResolveGridManager()
{
	if (GridManager)
	{
		return GridManager;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AGridManager> It(World); It; ++It)
	{
		// Fallback for Blueprint-spawned components that were not explicitly wired during pawn initialization.
		GridManager = *It;
		return GridManager;
	}

	return nullptr;
}

bool UTileEffectResolverComponent::ShouldConsumeEnteredTile() const
{
	return !TileAttributeEffectConfig || TileAttributeEffectConfig->bConsumeTileAfterEnter;
}

ETileType UTileEffectResolverComponent::GetConsumedTileResultType() const
{
	return TileAttributeEffectConfig ? TileAttributeEffectConfig->ConsumedTileResultType : ETileType::Minimal;
}
