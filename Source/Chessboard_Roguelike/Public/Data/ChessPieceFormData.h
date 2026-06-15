#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Transform/ChessTransformTypes.h"
#include "ChessPieceFormData.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UTexture2D;

UCLASS(BlueprintType)
class CHESSBOARD_ROGUELIKE_API UChessPieceFormData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform")
	EChessTransformType TransformType = EChessTransformType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|UI")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Visual")
	TObjectPtr<UStaticMesh> FormMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Visual")
	TObjectPtr<UMaterialInterface> FormMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Movement")
	TArray<FIntPoint> MoveDirections;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Movement", meta = (ClampMin = "1"))
	int32 MaxRange = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Movement")
	bool bCanJump = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Transform|Movement")
	bool bCanCaptureEnemy = true;
};
