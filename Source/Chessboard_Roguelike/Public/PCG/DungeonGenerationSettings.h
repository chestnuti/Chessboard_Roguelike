#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DungeonGenerationSettings.generated.h"

class AGridEnemyPawn;

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FDungeonEnemySpawnEntry
{
	GENERATED_BODY()

	// Enemy class selected when this entry passes depth filtering and weighted selection.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Spawns")
	TSubclassOf<AGridEnemyPawn> EnemyClass;

	// Relative selection chance among all entries valid for a candidate depth.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Spawns", meta = (ClampMin = "1"))
	int32 Weight = 1;

	// Inclusive depth range for this enemy type; use it to gate stronger enemies to later rooms.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Spawns", meta = (ClampMin = "0"))
	int32 MinDepth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Spawns", meta = (ClampMin = "0"))
	int32 MaxDepth = 999;

	// Optional base kill threshold for this enemy entry; 0 keeps the enemy Blueprint/class default.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Spawns", meta = (ClampMin = "0"))
	int32 KillThresholdOverride = 0;

	// Added to the resolved base threshold for every room-depth step of the spawn candidate.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Spawns", meta = (ClampMin = "0"))
	int32 KillThresholdBonusPerDepth = 0;
};

UCLASS(BlueprintType)
class CHESSBOARD_ROGUELIKE_API UDungeonGenerationSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UDungeonGenerationSettings();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Seed")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Bounds", meta = (ClampMin = "8"))
	int32 Width = 48;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Bounds", meta = (ClampMin = "8"))
	int32 Height = 32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|L-System")
	FString Axiom = TEXT("R[+F][-F]F");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|L-System", meta = (ClampMin = "0", ClampMax = "5"))
	int32 Iterations = 2;

	// Rule keys are single-character names such as F or R.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|L-System")
	TMap<FName, FString> ProductionRules;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Rooms", meta = (ClampMin = "1"))
	int32 MinRoomRadius = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Rooms", meta = (ClampMin = "1"))
	int32 MaxRoomRadius = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Rooms", meta = (ClampMin = "1"))
	int32 MaxRoomCount = 16;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Rooms", meta = (ClampMin = "0"))
	int32 BoundaryNoise = 2;

	// Extra center-distance padding used when placing new room nodes to reduce room overlap.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Rooms", meta = (ClampMin = "0"))
	int32 RoomSeparation = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Corridors", meta = (ClampMin = "1"))
	int32 CorridorWidth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Corridors", meta = (ClampMin = "0", ClampMax = "100"))
	int32 CorridorWanderChance = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Validation", meta = (ClampMin = "1"))
	int32 MaxGenerationAttempts = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Validation", meta = (ClampMin = "0"))
	int32 StartSafetyRadius = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Tiles", meta = (ClampMin = "0", ClampMax = "100"))
	int32 ConstructTileChance = 25;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Tiles", meta = (ClampMin = "0", ClampMax = "100"))
	int32 AcidTileChance = 25;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Spawns", meta = (ClampMin = "0"))
	int32 EnemySpawnCount = 4;

	// Runtime spawning consumes this pool; the layout generator only produces candidate coordinates.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Spawns")
	TArray<FDungeonEnemySpawnEntry> EnemySpawnPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Spawns", meta = (ClampMin = "0"))
	int32 EventCandidateCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Spawns", meta = (ClampMin = "0"))
	int32 RewardCandidateCount = 3;
};
