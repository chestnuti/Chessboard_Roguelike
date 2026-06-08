#include "Pickup/GridPickupManager.h"

#include "Components/SceneComponent.h"
#include "Pickup/GridPickupActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridPickupManager, Log, All);

AGridPickupManager::AGridPickupManager()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

bool AGridPickupManager::RegisterPickup(AGridPickupActor* Pickup)
{
	if (!Pickup)
	{
		UE_LOG(LogGridPickupManager, Warning, TEXT("RegisterPickup failed: Pickup is null."));
		return false;
	}

	const FIntPoint Coord = Pickup->GetGridCoord();
	if (PickupsByCoord.Contains(Coord))
	{
		UE_LOG(LogGridPickupManager, Warning, TEXT("RegisterPickup skipped %s: coord (%d,%d) already has a pickup."),
			*Pickup->GetName(), Coord.X, Coord.Y);
		return false;
	}

	PickupsByCoord.Add(Coord, Pickup);
	return true;
}

void AGridPickupManager::UnregisterPickupAt(FIntPoint Coord)
{
	PickupsByCoord.Remove(Coord);
}

AGridPickupActor* AGridPickupManager::GetPickupAt(FIntPoint Coord) const
{
	const TObjectPtr<AGridPickupActor>* PickupPtr = PickupsByCoord.Find(Coord);
	return PickupPtr ? PickupPtr->Get() : nullptr;
}

bool AGridPickupManager::TryCollectPickupAt(FIntPoint Coord, AGridPawn* PlayerPawn)
{
	AGridPickupActor* Pickup = GetPickupAt(Coord);
	if (!Pickup)
	{
		return false;
	}

	if (!IsValid(Pickup))
	{
		UnregisterPickupAt(Coord);
		return false;
	}

	const bool bConsumed = Pickup->TryCollect(PlayerPawn);
	if (bConsumed)
	{
		UnregisterPickupAt(Coord);
	}

	return bConsumed;
}

void AGridPickupManager::ClearAllPickups()
{
	for (const TPair<FIntPoint, TObjectPtr<AGridPickupActor>>& Pair : PickupsByCoord)
	{
		if (AGridPickupActor* Pickup = Pair.Value.Get())
		{
			Pickup->Destroy();
		}
	}

	PickupsByCoord.Reset();
}
