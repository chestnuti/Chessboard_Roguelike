#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemy/EnemyTypes.h"
#include "Grid/GridTypes.h"
#include "Transform/ChessTransformTypes.h"
#include "TutorialFlowData.generated.h"

UENUM(BlueprintType)
enum class ETutorialStepTriggerType : uint8
{
	Manual UMETA(DisplayName = "Manual"),
	PlayerSteppedOnTileType UMETA(DisplayName = "Player Stepped On Tile Type"),
	PlayerSteppedOnAnyAttributeTile UMETA(DisplayName = "Player Stepped On Any Attribute Tile"),
	AllAttributeTilesCleared UMETA(DisplayName = "All Attribute Tiles Cleared"),
	PlayerDamagedOrEnemyKilled UMETA(DisplayName = "Player Damaged Or Enemy Killed"),
	EnemyKilled UMETA(DisplayName = "Enemy Killed"),
	AllEnemiesCleared UMETA(DisplayName = "All Enemies Cleared"),
	ConversionEnergyUsed UMETA(DisplayName = "Conversion Energy Used"),
	AttributeReached UMETA(DisplayName = "Attribute Reached"),
	TransformWheelOpened UMETA(DisplayName = "Transform Wheel Opened"),
	TransformPieceCollectedCount UMETA(DisplayName = "Transform Piece Collected Count"),
	TransformMoveCompleted UMETA(DisplayName = "Transform Move Completed")
};

UENUM(BlueprintType)
enum class ETutorialAttributeKind : uint8
{
	Any UMETA(DisplayName = "Any"),
	Construct UMETA(DisplayName = "Construct"),
	Acid UMETA(DisplayName = "Acid")
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FTutorialStepDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	FName StepId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial", meta = (MultiLine = "true"))
	FText InstructionText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial")
	ETutorialStepTriggerType TriggerType = ETutorialStepTriggerType::Manual;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Condition")
	ETileType TargetTileType = ETileType::Minimal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Condition")
	ETutorialAttributeKind AttributeKind = ETutorialAttributeKind::Any;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Condition", meta = (ClampMin = "0"))
	int32 RequiredValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Condition")
	EChessTransformType TransformType = EChessTransformType::None;
};

UCLASS(BlueprintType)
class CHESSBOARD_ROGUELIKE_API UTutorialFlowDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FName FlowId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TArray<FTutorialStepDefinition> Steps;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Travel")
	FName NextLevelName = NAME_None;
};
