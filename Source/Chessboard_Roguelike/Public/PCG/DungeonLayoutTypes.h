#pragma once

#include "CoreMinimal.h"
#include "Grid/GridTypes.h"
#include "DungeonLayoutTypes.generated.h"

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FLSystemDungeonRoomNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	int32 RoomId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	FIntPoint Center = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	int32 ApproxRadius = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	int32 Depth = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	TArray<FIntPoint> Tiles;
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FLSystemDungeonConnection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	int32 FromRoomId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	int32 ToRoomId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	TArray<FIntPoint> CorridorPath;
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FDungeonSpawnCandidate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	FIntPoint Coord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	int32 RegionId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	int32 Depth = 0;
};

USTRUCT(BlueprintType)
struct CHESSBOARD_ROGUELIKE_API FGeneratedDungeonLayout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	int32 Width = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	int32 Height = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	TArray<FGridTileLayoutData> Tiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	TArray<FLSystemDungeonRoomNode> Rooms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	TArray<FLSystemDungeonConnection> Connections;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	FIntPoint StartCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	FIntPoint ExitCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	TArray<FDungeonSpawnCandidate> EnemySpawnCandidates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	TArray<FDungeonSpawnCandidate> EventSpawnCandidates;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG|Dungeon")
	TArray<FDungeonSpawnCandidate> RewardSpawnCandidates;
};
