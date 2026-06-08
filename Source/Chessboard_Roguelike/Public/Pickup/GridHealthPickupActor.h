#pragma once

#include "CoreMinimal.h"
#include "Pickup/GridPickupActor.h"
#include "GridHealthPickupActor.generated.h"

// Restores player HP through UPlayerAttributeComponent::Heal.
UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGridHealthPickupActor : public AGridPickupActor
{
	GENERATED_BODY()

public:
	AGridHealthPickupActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup|Health", meta = (ClampMin = "1"))
	int32 HealAmount = 1;

	virtual bool ApplyPickupEffect_Implementation(AGridPawn* PlayerPawn) override;
};
