#pragma once

#include "CoreMinimal.h"
#include "Grid/GridTypes.h"
#include "ChessTransformTypes.generated.h"

UENUM(BlueprintType)
enum class EChessTransformType : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Knight = 1 UMETA(DisplayName = "Knight"),
	Bishop = 2 UMETA(DisplayName = "Bishop"),
	Rook = 3 UMETA(DisplayName = "Rook"),
	Queen = 4 UMETA(DisplayName = "Queen")
};

UENUM(BlueprintType)
enum class ETransformTargetType : uint8
{
	Move = 0 UMETA(DisplayName = "Move"),
	Capture = 1 UMETA(DisplayName = "Capture")
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FTransformMoveTarget
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
	FIntPoint Coord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
	ETransformTargetType TargetType = ETransformTargetType::Move;
};
