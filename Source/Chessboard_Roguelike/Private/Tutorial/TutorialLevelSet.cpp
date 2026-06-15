#include "Tutorial/TutorialLevelSet.h"

namespace
{
	constexpr int32 TutorialGridWidth = 10;
	constexpr int32 TutorialGridHeight = 10;

	FGridTileLayoutData MakeTutorialTile(int32 X, int32 Y, ETileType TileType)
	{
		FGridTileLayoutData Tile;
		Tile.GridCoord = FIntPoint(X, Y);
		Tile.TileType = TileType;
		Tile.CellRole = EGridCellRole::Open;
		Tile.bWalkable = true;
		Tile.bConvertible = true;
		Tile.RegionId = 0;
		Tile.Depth = 0;
		return Tile;
	}

	void AddDefaultTiles(FTutorialLevelDefinition& Level)
	{
		Level.Width = TutorialGridWidth;
		Level.Height = TutorialGridHeight;
		Level.Tiles.Reset(TutorialGridWidth * TutorialGridHeight);

		for (int32 Y = 0; Y < TutorialGridHeight; ++Y)
		{
			for (int32 X = 0; X < TutorialGridWidth; ++X)
			{
				Level.Tiles.Add(MakeTutorialTile(X, Y, ETileType::Minimal));
			}
		}
	}

	void SetTileType(FTutorialLevelDefinition& Level, FIntPoint Coord, ETileType TileType)
	{
		const int32 TileIndex = Coord.Y * Level.Width + Coord.X;
		if (Level.Tiles.IsValidIndex(TileIndex))
		{
			Level.Tiles[TileIndex].TileType = TileType;
		}
	}

	FTutorialEnemySpawnData MakeEnemy(
		FIntPoint Coord,
		EEnemyFaction Faction,
		EEnemyBehaviorType BehaviorType,
		int32 KillThreshold,
		int32 MaxRangedAttackDistance = 0)
	{
		FTutorialEnemySpawnData Enemy;
		Enemy.Coord = Coord;
		Enemy.Faction = Faction;
		Enemy.BehaviorType = BehaviorType;
		Enemy.KillThreshold = KillThreshold;
		Enemy.MaxRangedAttackDistance = MaxRangedAttackDistance;
		return Enemy;
	}

	FTutorialPickupSpawnData MakePickup(FIntPoint Coord)
	{
		FTutorialPickupSpawnData Pickup;
		Pickup.Coord = Coord;
		return Pickup;
	}

	FTutorialLevelDefinition BuildTileAttributesLevel()
	{
		FTutorialLevelDefinition Level;
		Level.LevelId = TEXT("Tutorial_01_TileAttributes");
		Level.StartCoord = FIntPoint(1, 1);
		Level.ExitCoord = FIntPoint(8, 8);
		AddDefaultTiles(Level);

		SetTileType(Level, FIntPoint(2, 1), ETileType::Construct);
		SetTileType(Level, FIntPoint(3, 1), ETileType::Construct);
		SetTileType(Level, FIntPoint(4, 1), ETileType::Acid);
		SetTileType(Level, FIntPoint(5, 1), ETileType::Acid);
		SetTileType(Level, FIntPoint(2, 3), ETileType::Construct);
		SetTileType(Level, FIntPoint(3, 3), ETileType::Acid);
		SetTileType(Level, FIntPoint(4, 4), ETileType::Construct);
		SetTileType(Level, FIntPoint(5, 5), ETileType::Acid);
		SetTileType(Level, FIntPoint(7, 6), ETileType::Construct);
		SetTileType(Level, FIntPoint(8, 7), ETileType::Acid);

		return Level;
	}

	FTutorialLevelDefinition BuildEnemyKillLevel()
	{
		FTutorialLevelDefinition Level;
		Level.LevelId = TEXT("Tutorial_02_EnemyKills");
		Level.StartCoord = FIntPoint(1, 1);
		Level.ExitCoord = FIntPoint(8, 8);
		AddDefaultTiles(Level);

		SetTileType(Level, FIntPoint(2, 1), ETileType::Construct);
		SetTileType(Level, FIntPoint(3, 1), ETileType::Construct);
		SetTileType(Level, FIntPoint(1, 2), ETileType::Acid);
		SetTileType(Level, FIntPoint(1, 3), ETileType::Acid);
		SetTileType(Level, FIntPoint(4, 3), ETileType::Construct);
		SetTileType(Level, FIntPoint(5, 4), ETileType::Acid);

		Level.Enemies.Add(MakeEnemy(FIntPoint(6, 2), EEnemyFaction::Construct, EEnemyBehaviorType::Melee, 2));
		Level.Enemies.Add(MakeEnemy(FIntPoint(2, 6), EEnemyFaction::Acid, EEnemyBehaviorType::Melee, 2));
		return Level;
	}

	FTutorialLevelDefinition BuildConversionEnergyLevel()
	{
		FTutorialLevelDefinition Level;
		Level.LevelId = TEXT("Tutorial_03_ConversionEnergy");
		Level.StartCoord = FIntPoint(1, 1);
		Level.ExitCoord = FIntPoint(8, 8);
		AddDefaultTiles(Level);

		SetTileType(Level, FIntPoint(2, 1), ETileType::Construct);
		SetTileType(Level, FIntPoint(5, 4), ETileType::Acid);
		SetTileType(Level, FIntPoint(6, 5), ETileType::Acid);
		SetTileType(Level, FIntPoint(7, 5), ETileType::Acid);

		Level.Enemies.Add(MakeEnemy(FIntPoint(3, 1), EEnemyFaction::Acid, EEnemyBehaviorType::Melee, 1));
		Level.Enemies.Add(MakeEnemy(FIntPoint(8, 4), EEnemyFaction::Construct, EEnemyBehaviorType::Melee, 3));
		Level.Enemies.Add(MakeEnemy(FIntPoint(8, 6), EEnemyFaction::Acid, EEnemyBehaviorType::Melee, 3));
		return Level;
	}

	FTutorialLevelDefinition BuildRangedFriendlyFireLevel()
	{
		FTutorialLevelDefinition Level = BuildEnemyKillLevel();
		Level.LevelId = TEXT("Tutorial_04_RangedFriendlyFire");

		SetTileType(Level, FIntPoint(4, 6), ETileType::Construct);
		SetTileType(Level, FIntPoint(6, 6), ETileType::Acid);
		SetTileType(Level, FIntPoint(5, 7), ETileType::Construct);
		SetTileType(Level, FIntPoint(7, 7), ETileType::Acid);

		Level.Enemies.Add(MakeEnemy(FIntPoint(8, 1), EEnemyFaction::Construct, EEnemyBehaviorType::Ranged, 2, 8));
		Level.Enemies.Add(MakeEnemy(FIntPoint(1, 8), EEnemyFaction::Acid, EEnemyBehaviorType::Ranged, 2, 8));
		return Level;
	}

	FTutorialLevelDefinition BuildFactionSuppressionLevel()
	{
		FTutorialLevelDefinition Level;
		Level.LevelId = TEXT("Tutorial_05_FactionSuppression");
		Level.StartCoord = FIntPoint(5, 5);
		Level.ExitCoord = FIntPoint(9, 5);
		AddDefaultTiles(Level);

		for (int32 X = 0; X < TutorialGridWidth; ++X)
		{
			SetTileType(Level, FIntPoint(X, 0), ETileType::Acid);
			SetTileType(Level, FIntPoint(X, 1), ETileType::Acid);
			SetTileType(Level, FIntPoint(X, 8), ETileType::Construct);
			SetTileType(Level, FIntPoint(X, 9), ETileType::Construct);
		}

		Level.Enemies.Add(MakeEnemy(FIntPoint(0, 2), EEnemyFaction::Acid, EEnemyBehaviorType::Melee, 2));
		Level.Enemies.Add(MakeEnemy(FIntPoint(0, 9), EEnemyFaction::Construct, EEnemyBehaviorType::Melee, 2));
		return Level;
	}

	FTutorialLevelDefinition BuildHealthAndTransformLevel()
	{
		FTutorialLevelDefinition Level;
		Level.LevelId = TEXT("Tutorial_06_HealthAndTransform");
		Level.StartCoord = FIntPoint(1, 1);
		Level.ExitCoord = FIntPoint(8, 8);
		AddDefaultTiles(Level);

		SetTileType(Level, FIntPoint(2, 1), ETileType::Construct);
		SetTileType(Level, FIntPoint(3, 2), ETileType::Construct);
		SetTileType(Level, FIntPoint(5, 2), ETileType::Acid);
		SetTileType(Level, FIntPoint(1, 3), ETileType::Acid);
		SetTileType(Level, FIntPoint(3, 5), ETileType::Construct);
		SetTileType(Level, FIntPoint(6, 6), ETileType::Acid);
		SetTileType(Level, FIntPoint(7, 7), ETileType::Construct);

		Level.Pickups.Add(MakePickup(FIntPoint(2, 1)));
		Level.Pickups.Add(MakePickup(FIntPoint(1, 3)));
		Level.Pickups.Add(MakePickup(FIntPoint(7, 7)));
		Level.Pickups.Add(MakePickup(FIntPoint(3, 2)));
		Level.Pickups.Add(MakePickup(FIntPoint(5, 2)));
		Level.Pickups.Add(MakePickup(FIntPoint(3, 5)));
		Level.Pickups.Add(MakePickup(FIntPoint(6, 6)));
		return Level;
	}
}

UTutorialLevelSet::UTutorialLevelSet()
{
	TutorialLevels.Add(BuildTileAttributesLevel());
	TutorialLevels.Add(BuildEnemyKillLevel());
	TutorialLevels.Add(BuildConversionEnergyLevel());
	TutorialLevels.Add(BuildRangedFriendlyFireLevel());
	TutorialLevels.Add(BuildFactionSuppressionLevel());
	TutorialLevels.Add(BuildHealthAndTransformLevel());
}
