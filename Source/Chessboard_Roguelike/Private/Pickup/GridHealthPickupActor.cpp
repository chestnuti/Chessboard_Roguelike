#include "Pickup/GridHealthPickupActor.h"

#include "Player/GridPawn.h"
#include "Player/PlayerAttributeComponent.h"

AGridHealthPickupActor::AGridHealthPickupActor()
{
	PickupId = TEXT("Health");
	HealAmount = 1;
	bConsumeOnSuccessfulEffect = true;
	bConsumeWhenEffectFails = false;
}

bool AGridHealthPickupActor::ApplyPickupEffect_Implementation(AGridPawn* PlayerPawn)
{
	if (!PlayerPawn)
	{
		return false;
	}

	UPlayerAttributeComponent* AttributeComponent = PlayerPawn->PlayerAttributeComponent;
	if (!AttributeComponent)
	{
		AttributeComponent = PlayerPawn->FindComponentByClass<UPlayerAttributeComponent>();
	}

	return AttributeComponent ? AttributeComponent->Heal(HealAmount) : false;
}
