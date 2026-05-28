#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridSettings.generated.h"

class UStaticMesh;

UCLASS(BlueprintType)
class CHESSBOARD_ROGUELIKE_API UGridSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 Width = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 Height = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	float TileSize = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TObjectPtr<UStaticMesh> TileMesh;
};
