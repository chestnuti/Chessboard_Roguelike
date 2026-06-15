#include "Pickup/GridTransformPiecePickupActor.h"

#include "Player/GridPawn.h"
#include "Player/PlayerTransformInventoryComponent.h"

AGridTransformPiecePickupActor::AGridTransformPiecePickupActor()
{
	PickupId = TEXT("TransformPiece");
	bConsumeOnSuccessfulEffect = true;
	bConsumeWhenEffectFails = false;
}

bool AGridTransformPiecePickupActor::ApplyPickupEffect_Implementation(AGridPawn* PlayerPawn)
{
	if (!PlayerPawn || !PlayerPawn->TransformInventoryComponent)
	{
		return false;
	}

	return PlayerPawn->TransformInventoryComponent->AddTransformPiece(TransformType, Amount);
}
