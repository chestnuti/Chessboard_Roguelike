#pragma once

#include "CoreMinimal.h"
#include "Pickup/GridPickupActor.h"
#include "Transform/ChessTransformTypes.h"
#include "GridTransformPiecePickupActor.generated.h"

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGridTransformPiecePickupActor : public AGridPickupActor
{
	GENERATED_BODY()

public:
	AGridTransformPiecePickupActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup|Transform")
	EChessTransformType TransformType = EChessTransformType::Knight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup|Transform", meta = (ClampMin = "1"))
	int32 Amount = 1;

	virtual bool ApplyPickupEffect_Implementation(AGridPawn* PlayerPawn) override;
};
