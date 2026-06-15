#pragma once

#include "CoreMinimal.h"
#include "Enemy/EnemyTypes.h"
#include "Engine/DataAsset.h"
#include "Grid/GridTypes.h"
#include "TutorialLevelSet.generated.h"

class AGridEnemyPawn;
class AGridPickupActor;

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FTutorialEnemySpawnData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Enemy")
	FIntPoint Coord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Enemy")
	TSubclassOf<AGridEnemyPawn> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Enemy")
	EEnemyFaction Faction = EEnemyFaction::Construct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Enemy")
	EEnemyBehaviorType BehaviorType = EEnemyBehaviorType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Enemy", meta = (ClampMin = "1"))
	int32 KillThreshold = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Enemy", meta = (ClampMin = "0"))
	int32 MaxRangedAttackDistance = 0;
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FTutorialPickupSpawnData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Pickup")
	FIntPoint Coord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Pickup")
	TSubclassOf<AGridPickupActor> PickupClass;
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FTutorialLevelDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Level")
	FName LevelId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Level", meta = (ClampMin = "1"))
	int32 Width = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Level", meta = (ClampMin = "1"))
	int32 Height = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Level")
	FIntPoint StartCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Level")
	FIntPoint ExitCoord = FIntPoint(9, 9);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Level")
	TArray<FGridTileLayoutData> Tiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Level")
	TArray<FTutorialEnemySpawnData> Enemies;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tutorial|Level")
	TArray<FTutorialPickupSpawnData> Pickups;
};

UCLASS(BlueprintType)
class CHESSBOARD_ROGUELIKE_API UTutorialLevelSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UTutorialLevelSet();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TArray<FTutorialLevelDefinition> TutorialLevels;
};
