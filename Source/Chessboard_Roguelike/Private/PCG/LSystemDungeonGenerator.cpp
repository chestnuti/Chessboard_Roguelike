#include "PCG/LSystemDungeonGenerator.h"

DEFINE_LOG_CATEGORY_STATIC(LogLSystemDungeonGenerator, Log, All);

namespace
{
	struct FTurtleState
	{
		FIntPoint Coord = FIntPoint::ZeroValue;
		FIntPoint Direction = FIntPoint(1, 0);
		int32 RoomId = INDEX_NONE;
		int32 Depth = 0;
	};

	bool IsInBounds(const FIntPoint& Coord, int32 Width, int32 Height)
	{
		return Coord.X >= 0 && Coord.Y >= 0 && Coord.X < Width && Coord.Y < Height;
	}

	int32 GetTileIndex(const FIntPoint& Coord, int32 Width)
	{
		return Coord.Y * Width + Coord.X;
	}

	FIntPoint RotateRight(const FIntPoint& Direction)
	{
		return FIntPoint(Direction.Y, -Direction.X);
	}

	FIntPoint RotateLeft(const FIntPoint& Direction)
	{
		return FIntPoint(-Direction.Y, Direction.X);
	}

	FIntPoint ClampRoomCenter(const FIntPoint& Coord, const UDungeonGenerationSettings& Settings)
	{
		const int32 Padding = FMath::Max(2, Settings.MaxRoomRadius + Settings.CorridorWidth + 1);
		return FIntPoint(
			FMath::Clamp(Coord.X, Padding, FMath::Max(Padding, Settings.Width - Padding - 1)),
			FMath::Clamp(Coord.Y, Padding, FMath::Max(Padding, Settings.Height - Padding - 1)));
	}

	int32 MakeRoomRadius(const UDungeonGenerationSettings& Settings, FRandomStream& Stream)
	{
		return Stream.RandRange(Settings.MinRoomRadius, FMath::Max(Settings.MinRoomRadius, Settings.MaxRoomRadius));
	}

	bool IsRoomPlacementClear(
		const FGeneratedDungeonLayout& Layout,
		const UDungeonGenerationSettings& Settings,
		const FIntPoint& Center,
		int32 Radius)
	{
		const int32 Clearance = FMath::Max(0, Settings.BoundaryNoise + Settings.RoomSeparation);
		for (const FLSystemDungeonRoomNode& ExistingRoom : Layout.Rooms)
		{
			const float Distance = FVector2D(
				static_cast<float>(Center.X - ExistingRoom.Center.X),
				static_cast<float>(Center.Y - ExistingRoom.Center.Y)).Length();
			const float MinimumDistance = static_cast<float>(ExistingRoom.ApproxRadius + Radius + Clearance);
			if (Distance < MinimumDistance)
			{
				return false;
			}
		}

		return true;
	}

	FString ExpandLSystem(const UDungeonGenerationSettings& Settings)
	{
		FString Expanded = Settings.Axiom;
		for (int32 Iteration = 0; Iteration < Settings.Iterations; ++Iteration)
		{
			FString Next;
			Next.Reserve(Expanded.Len() * 2);

			for (int32 Index = 0; Index < Expanded.Len(); ++Index)
			{
				const FString SymbolString = FString::Chr(Expanded[Index]);
				if (const FString* Rule = Settings.ProductionRules.Find(FName(*SymbolString)))
				{
					Next += *Rule;
				}
				else
				{
					Next.AppendChar(Expanded[Index]);
				}
			}

			Expanded = Next;
		}

		return Expanded;
	}

	void InitializeWallLayout(const UDungeonGenerationSettings& Settings, FGeneratedDungeonLayout& Layout)
	{
		// Start from an all-wall raster so every carved floor tile is explicitly authored by the generator.
		Layout.Width = Settings.Width;
		Layout.Height = Settings.Height;
		Layout.Seed = Settings.Seed;
		Layout.Tiles.Reset();
		Layout.Tiles.SetNum(Settings.Width * Settings.Height);
		Layout.EnemySpawnCandidates.Reset();
		Layout.EventSpawnCandidates.Reset();
		Layout.RewardSpawnCandidates.Reset();

		for (int32 Y = 0; Y < Settings.Height; ++Y)
		{
			for (int32 X = 0; X < Settings.Width; ++X)
			{
				FGridTileLayoutData& Tile = Layout.Tiles[GetTileIndex(FIntPoint(X, Y), Settings.Width)];
				Tile.GridCoord = FIntPoint(X, Y);
				Tile.TileType = ETileType::Obstacle;
				Tile.CellRole = EGridCellRole::Wall;
				Tile.bWalkable = false;
				Tile.bConvertible = false;
				Tile.RegionId = INDEX_NONE;
				Tile.Depth = 0;
			}
		}
	}

	int32 AddRoomNode(FGeneratedDungeonLayout& Layout, const UDungeonGenerationSettings& Settings, const FIntPoint& Center, int32 Depth, int32 Radius)
	{
		FLSystemDungeonRoomNode Room;
		Room.RoomId = Layout.Rooms.Num();
		Room.Center = ClampRoomCenter(Center, Settings);
		Room.ApproxRadius = Radius;
		Room.Depth = Depth;
		Layout.Rooms.Add(Room);
		return Room.RoomId;
	}

	int32 AdvanceAndAddRoom(FGeneratedDungeonLayout& Layout, const UDungeonGenerationSettings& Settings, FRandomStream& Stream, FTurtleState& Turtle)
	{
		const FIntPoint Perpendicular = RotateRight(Turtle.Direction);
		const int32 PreviousRoomId = Turtle.RoomId;
		const int32 CandidateRadius = MakeRoomRadius(Settings, Stream);
		const int32 BaseMinStepDistance = Settings.MaxRoomRadius * 2 + 2;
		const int32 BaseMaxStepDistance = Settings.MaxRoomRadius * 3 + 4;
		constexpr int32 MaxPlacementAttempts = 24;

		for (int32 Attempt = 0; Attempt < MaxPlacementAttempts; ++Attempt)
		{
			const int32 StepDistance = Stream.RandRange(BaseMinStepDistance, BaseMaxStepDistance + Attempt * FMath::Max(1, Settings.MaxRoomRadius));
			const int32 JitterRange = 2 + Attempt;
			const FIntPoint ForwardStep(Turtle.Direction.X * StepDistance, Turtle.Direction.Y * StepDistance);
			const FIntPoint Jitter(
				Perpendicular.X * Stream.RandRange(-JitterRange, JitterRange),
				Perpendicular.Y * Stream.RandRange(-JitterRange, JitterRange));
			const FIntPoint NextCenter = ClampRoomCenter(Turtle.Coord + ForwardStep + Jitter, Settings);

			if (!IsRoomPlacementClear(Layout, Settings, NextCenter, CandidateRadius))
			{
				continue;
			}

			const int32 NextRoomId = AddRoomNode(Layout, Settings, NextCenter, Turtle.Depth + 1, CandidateRadius);
			if (Layout.Rooms.IsValidIndex(PreviousRoomId))
			{
				FLSystemDungeonConnection Connection;
				Connection.FromRoomId = PreviousRoomId;
				Connection.ToRoomId = NextRoomId;
				Layout.Connections.Add(Connection);
			}

			Turtle.Coord = Layout.Rooms[NextRoomId].Center;
			Turtle.RoomId = NextRoomId;
			Turtle.Depth = Layout.Rooms[NextRoomId].Depth;
			return NextRoomId;
		}

		UE_LOG(LogLSystemDungeonGenerator, Verbose, TEXT("Skipped room at depth %d: no non-overlapping placement found after %d attempts."),
			Turtle.Depth + 1, MaxPlacementAttempts);
		return INDEX_NONE;
	}

	bool BuildRoomGraph(const UDungeonGenerationSettings& Settings, FRandomStream& Stream, FGeneratedDungeonLayout& Layout)
	{
		// Stage 1: interpret the expanded L-system as a room graph, without touching rasterized tiles.
		Layout.Rooms.Reset();
		Layout.Connections.Reset();
		Layout.StartCoord = FIntPoint::ZeroValue;
		Layout.ExitCoord = FIntPoint::ZeroValue;

		FTurtleState Turtle;
		Turtle.Coord = FIntPoint(Settings.Width / 2, Settings.Height / 2);
		Turtle.RoomId = AddRoomNode(Layout, Settings, Turtle.Coord, 0, MakeRoomRadius(Settings, Stream));
		Layout.StartCoord = Layout.Rooms[Turtle.RoomId].Center;

		TArray<FTurtleState> Stack;
		const FString ExpandedRules = ExpandLSystem(Settings);

		for (int32 SymbolIndex = 0; SymbolIndex < ExpandedRules.Len() && Layout.Rooms.Num() < Settings.MaxRoomCount; ++SymbolIndex)
		{
			const TCHAR Symbol = ExpandedRules[SymbolIndex];
			switch (Symbol)
			{
			case TCHAR('F'):
			case TCHAR('B'):
			case TCHAR('E'):
				AdvanceAndAddRoom(Layout, Settings, Stream, Turtle);
				break;
			case TCHAR('R'):
				if (Turtle.RoomId == INDEX_NONE)
				{
					const int32 CandidateRadius = MakeRoomRadius(Settings, Stream);
					if (IsRoomPlacementClear(Layout, Settings, Turtle.Coord, CandidateRadius))
					{
						Turtle.RoomId = AddRoomNode(Layout, Settings, Turtle.Coord, Turtle.Depth, CandidateRadius);
					}
				}
				break;
			case TCHAR('+'):
				Turtle.Direction = RotateRight(Turtle.Direction);
				break;
			case TCHAR('-'):
				Turtle.Direction = RotateLeft(Turtle.Direction);
				break;
			case TCHAR('['):
				Stack.Push(Turtle);
				break;
			case TCHAR(']'):
				if (!Stack.IsEmpty())
				{
					Turtle = Stack.Pop(EAllowShrinking::No);
				}
				break;
			default:
				break;
			}
		}

		if (Layout.Rooms.Num() > 1)
		{
			int32 ExitRoomId = 0;
			int32 BestDepth = MIN_int32;
			for (const FLSystemDungeonRoomNode& Room : Layout.Rooms)
			{
				if (Room.Depth > BestDepth)
				{
					BestDepth = Room.Depth;
					ExitRoomId = Room.RoomId;
				}
			}
			Layout.ExitCoord = Layout.Rooms[ExitRoomId].Center;
		}

		return Layout.Rooms.Num() > 0;
	}

	bool StampTile(FGeneratedDungeonLayout& Layout, const FIntPoint& Coord, EGridCellRole Role, int32 RegionId, int32 Depth)
	{
		if (!IsInBounds(Coord, Layout.Width, Layout.Height))
		{
			return false;
		}

		FGridTileLayoutData& Tile = Layout.Tiles[GetTileIndex(Coord, Layout.Width)];
		// Corridors should connect rooms without erasing room/start/exit semantic labels.
		if (Role == EGridCellRole::Corridor
			&& (Tile.CellRole == EGridCellRole::Room || Tile.CellRole == EGridCellRole::Start || Tile.CellRole == EGridCellRole::Exit))
		{
			Tile.bWalkable = true;
			return false;
		}

		// When room masks overlap, keep the lower-depth ownership so deep clamped rooms cannot pollute early areas.
		if (Role == EGridCellRole::Room)
		{
			if (Tile.CellRole == EGridCellRole::Start || Tile.CellRole == EGridCellRole::Exit)
			{
				Tile.bWalkable = true;
				return false;
			}

			if (Tile.CellRole == EGridCellRole::Room && Tile.Depth <= Depth)
			{
				Tile.bWalkable = true;
				return false;
			}
		}

		Tile.TileType = ETileType::Minimal;
		Tile.CellRole = Role;
		Tile.bWalkable = true;
		Tile.bConvertible = true;
		Tile.RegionId = RegionId;
		Tile.Depth = Depth;
		return true;
	}

	void StampBrush(FGeneratedDungeonLayout& Layout, const FIntPoint& Center, int32 Radius, EGridCellRole Role, int32 RegionId, int32 Depth)
	{
		const int32 ClampedRadius = FMath::Max(0, Radius);
		for (int32 Y = Center.Y - ClampedRadius; Y <= Center.Y + ClampedRadius; ++Y)
		{
			for (int32 X = Center.X - ClampedRadius; X <= Center.X + ClampedRadius; ++X)
			{
				StampTile(Layout, FIntPoint(X, Y), Role, RegionId, Depth);
			}
		}
	}

	void RasterizeRoom(FGeneratedDungeonLayout& Layout, const UDungeonGenerationSettings& Settings, FRandomStream& Stream, FLSystemDungeonRoomNode& Room, EGridCellRole Role)
	{
		// BoundaryNoise perturbs the circular room cutoff to produce rough, cave-like room edges.
		Room.Tiles.Reset();
		for (int32 Y = Room.Center.Y - Room.ApproxRadius - Settings.BoundaryNoise; Y <= Room.Center.Y + Room.ApproxRadius + Settings.BoundaryNoise; ++Y)
		{
			for (int32 X = Room.Center.X - Room.ApproxRadius - Settings.BoundaryNoise; X <= Room.Center.X + Room.ApproxRadius + Settings.BoundaryNoise; ++X)
			{
				const FIntPoint Coord(X, Y);
				if (!IsInBounds(Coord, Layout.Width, Layout.Height))
				{
					continue;
				}

				const float Distance = FVector2D(
					static_cast<float>(Coord.X - Room.Center.X),
					static_cast<float>(Coord.Y - Room.Center.Y)).Length();
				const int32 BoundaryOffset = Stream.RandRange(-Settings.BoundaryNoise, Settings.BoundaryNoise);
				if (Distance <= static_cast<float>(Room.ApproxRadius + BoundaryOffset))
				{
					if (StampTile(Layout, Coord, Role, Room.RoomId, Room.Depth))
					{
						Room.Tiles.Add(Coord);
					}
				}
			}
		}
	}

	TArray<FIntPoint> CarveCorridor(FGeneratedDungeonLayout& Layout, const UDungeonGenerationSettings& Settings, FRandomStream& Stream, const FIntPoint& From, const FIntPoint& To, int32 RegionId, int32 Depth)
	{
		// Corridors bias toward the target but occasionally wander to avoid perfectly straight connections.
		TArray<FIntPoint> Path;
		FIntPoint Current = From;
		const int32 GuardLimit = (Settings.Width + Settings.Height) * 4;

		for (int32 Guard = 0; Current != To && Guard < GuardLimit; ++Guard)
		{
			Path.Add(Current);
			StampBrush(Layout, Current, Settings.CorridorWidth - 1, EGridCellRole::Corridor, RegionId, Depth);

			const FIntPoint Delta = To - Current;
			FIntPoint Step = FIntPoint::ZeroValue;
			const bool bCanWander = Stream.RandRange(0, 99) < Settings.CorridorWanderChance;

			if (bCanWander && (Delta.X != 0 || Delta.Y != 0))
			{
				const bool bHorizontalWander = Stream.RandRange(0, 1) == 0;
				Step = bHorizontalWander ? FIntPoint(0, Stream.RandRange(0, 1) == 0 ? -1 : 1) : FIntPoint(Stream.RandRange(0, 1) == 0 ? -1 : 1, 0);
			}
			else if (FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y) && Delta.X != 0)
			{
				Step = FIntPoint(FMath::Sign(Delta.X), 0);
			}
			else if (Delta.Y != 0)
			{
				Step = FIntPoint(0, FMath::Sign(Delta.Y));
			}
			else if (Delta.X != 0)
			{
				Step = FIntPoint(FMath::Sign(Delta.X), 0);
			}

			if (Step == FIntPoint::ZeroValue)
			{
				break;
			}

			Current = FIntPoint(
				FMath::Clamp(Current.X + Step.X, 1, Settings.Width - 2),
				FMath::Clamp(Current.Y + Step.Y, 1, Settings.Height - 2));
		}

		Path.Add(To);
		StampBrush(Layout, To, Settings.CorridorWidth - 1, EGridCellRole::Corridor, RegionId, Depth);
		return Path;
	}

	bool RasterizeLayout(const UDungeonGenerationSettings& Settings, FRandomStream& Stream, FGeneratedDungeonLayout& Layout)
	{
		// Stage 2: convert the abstract room graph into tile layout data consumable by GridManager.
		InitializeWallLayout(Settings, Layout);

		for (FLSystemDungeonRoomNode& Room : Layout.Rooms)
		{
			RasterizeRoom(Layout, Settings, Stream, Room, EGridCellRole::Room);
		}

		for (FLSystemDungeonConnection& Connection : Layout.Connections)
		{
			if (!Layout.Rooms.IsValidIndex(Connection.FromRoomId) || !Layout.Rooms.IsValidIndex(Connection.ToRoomId))
			{
				continue;
			}

			Connection.CorridorPath = CarveCorridor(
				Layout,
				Settings,
				Stream,
				Layout.Rooms[Connection.FromRoomId].Center,
				Layout.Rooms[Connection.ToRoomId].Center,
				Connection.ToRoomId,
				Layout.Rooms[Connection.ToRoomId].Depth);
		}

		if (IsInBounds(Layout.StartCoord, Layout.Width, Layout.Height))
		{
			StampBrush(Layout, Layout.StartCoord, Settings.StartSafetyRadius, EGridCellRole::Start, 0, 0);
		}

		if (IsInBounds(Layout.ExitCoord, Layout.Width, Layout.Height))
		{
			int32 ExitRegionId = INDEX_NONE;
			int32 ExitDepth = 0;
			for (const FLSystemDungeonRoomNode& Room : Layout.Rooms)
			{
				if (Room.Center == Layout.ExitCoord)
				{
					ExitRegionId = Room.RoomId;
					ExitDepth = Room.Depth;
					break;
				}
			}
			StampBrush(Layout, Layout.ExitCoord, 1, EGridCellRole::Exit, ExitRegionId, ExitDepth);
		}

		return true;
	}

	bool IsAttributeTileEligible(const FGridTileLayoutData& Tile)
	{
		return Tile.bWalkable
			&& Tile.bConvertible
			&& Tile.CellRole != EGridCellRole::Wall
			&& Tile.CellRole != EGridCellRole::Start
			&& Tile.CellRole != EGridCellRole::Exit;
	}

	void EnsureAttributeTileCoverage(
		const UDungeonGenerationSettings& Settings,
		FRandomStream& Stream,
		FGeneratedDungeonLayout& Layout,
		bool bHasConstructTile,
		bool bHasAcidTile)
	{
		TArray<int32> EligibleTileIndices;
		for (int32 TileIndex = 0; TileIndex < Layout.Tiles.Num(); ++TileIndex)
		{
			if (IsAttributeTileEligible(Layout.Tiles[TileIndex]))
			{
				EligibleTileIndices.Add(TileIndex);
			}
		}

		if (EligibleTileIndices.IsEmpty())
		{
			return;
		}

		if (!bHasConstructTile && Settings.ConstructTileChance > 0)
		{
			const int32 SelectedIndex = EligibleTileIndices[Stream.RandRange(0, EligibleTileIndices.Num() - 1)];
			Layout.Tiles[SelectedIndex].TileType = ETileType::Construct;
			bHasConstructTile = true;
		}

		if (!bHasAcidTile && Settings.AcidTileChance > 0)
		{
			int32 SelectedIndex = EligibleTileIndices[Stream.RandRange(0, EligibleTileIndices.Num() - 1)];
			if (EligibleTileIndices.Num() > 1)
			{
				for (int32 Attempt = 0; Attempt < EligibleTileIndices.Num(); ++Attempt)
				{
					const int32 CandidateIndex = EligibleTileIndices[Stream.RandRange(0, EligibleTileIndices.Num() - 1)];
					if (Layout.Tiles[CandidateIndex].TileType != ETileType::Construct)
					{
						SelectedIndex = CandidateIndex;
						break;
					}
				}
			}

			Layout.Tiles[SelectedIndex].TileType = ETileType::Acid;
		}
	}

	void AssignTileTypes(const UDungeonGenerationSettings& Settings, FRandomStream& Stream, FGeneratedDungeonLayout& Layout)
	{
		// Stage 3: assign gameplay resource types after map shape is final.
		const int32 ConstructChance = FMath::Clamp(Settings.ConstructTileChance, 0, 100);
		const int32 AcidChance = FMath::Clamp(Settings.AcidTileChance, 0, 100 - ConstructChance);
		bool bHasConstructTile = false;
		bool bHasAcidTile = false;

		for (FGridTileLayoutData& Tile : Layout.Tiles)
		{
			if (Tile.CellRole == EGridCellRole::Wall || !Tile.bWalkable)
			{
				Tile.TileType = ETileType::Obstacle;
				Tile.bConvertible = false;
				continue;
			}

			if (Tile.CellRole == EGridCellRole::Start)
			{
				Tile.TileType = ETileType::Minimal;
				Tile.bConvertible = true;
				continue;
			}

			if (Tile.CellRole == EGridCellRole::Exit)
			{
				Tile.TileType = ETileType::Minimal;
				Tile.bConvertible = true;
				continue;
			}

			const int32 Roll = Stream.RandRange(0, 99);
			if (Roll < ConstructChance)
			{
				Tile.TileType = ETileType::Construct;
				bHasConstructTile = true;
			}
			else if (Roll < ConstructChance + AcidChance)
			{
				Tile.TileType = ETileType::Acid;
				bHasAcidTile = true;
			}
			else
			{
				Tile.TileType = ETileType::Minimal;
			}
		}

		EnsureAttributeTileCoverage(Settings, Stream, Layout, bHasConstructTile, bHasAcidTile);
	}

	bool IsCandidateSafeFromStart(const FIntPoint& Coord, const UDungeonGenerationSettings& Settings, const FGeneratedDungeonLayout& Layout)
	{
		const int32 SafetyDistance = Settings.StartSafetyRadius;
		return FMath::Abs(Coord.X - Layout.StartCoord.X) + FMath::Abs(Coord.Y - Layout.StartCoord.Y) > SafetyDistance;
	}

	bool BuildReachableTileSet(const FGeneratedDungeonLayout& Layout, TSet<FIntPoint>& OutReachableTiles)
	{
		OutReachableTiles.Reset();
		if (Layout.Width <= 0 || Layout.Height <= 0 || Layout.Tiles.Num() != Layout.Width * Layout.Height || !IsInBounds(Layout.StartCoord, Layout.Width, Layout.Height))
		{
			return false;
		}

		const FGridTileLayoutData& StartTile = Layout.Tiles[GetTileIndex(Layout.StartCoord, Layout.Width)];
		if (!StartTile.bWalkable || StartTile.CellRole == EGridCellRole::Wall || StartTile.TileType == ETileType::Obstacle)
		{
			return false;
		}

		TArray<FIntPoint> Queue;
		OutReachableTiles.Add(Layout.StartCoord);
		Queue.Add(Layout.StartCoord);

		for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
		{
			const FIntPoint Current = Queue[QueueIndex];
			const FIntPoint Neighbors[] = {
				Current + FIntPoint(1, 0),
				Current + FIntPoint(-1, 0),
				Current + FIntPoint(0, 1),
				Current + FIntPoint(0, -1)
			};

			for (const FIntPoint& Neighbor : Neighbors)
			{
				if (!IsInBounds(Neighbor, Layout.Width, Layout.Height) || OutReachableTiles.Contains(Neighbor))
				{
					continue;
				}

				const FGridTileLayoutData& Tile = Layout.Tiles[GetTileIndex(Neighbor, Layout.Width)];
				if (!Tile.bWalkable || Tile.CellRole == EGridCellRole::Wall || Tile.TileType == ETileType::Obstacle)
				{
					continue;
				}

				OutReachableTiles.Add(Neighbor);
				Queue.Add(Neighbor);
			}
		}

		return true;
	}

	void AddCandidate(TArray<FDungeonSpawnCandidate>& Candidates, const FGridTileLayoutData& Tile)
	{
		FDungeonSpawnCandidate Candidate;
		Candidate.Coord = Tile.GridCoord;
		Candidate.RegionId = Tile.RegionId;
		Candidate.Depth = Tile.Depth;
		Candidates.Add(Candidate);
	}

	void BuildSpawnCandidates(const UDungeonGenerationSettings& Settings, FRandomStream& Stream, FGeneratedDungeonLayout& Layout)
	{
		// Stage 4: produce data-only spawn candidates; actual Actor spawning belongs to DungeonRunManager.
		TSet<FIntPoint> ReachableTiles;
		if (!BuildReachableTileSet(Layout, ReachableTiles))
		{
			UE_LOG(LogLSystemDungeonGenerator, Verbose, TEXT("BuildSpawnCandidates skipped: no reachable tile set for seed=%d."), Layout.Seed);
			return;
		}

		TArray<FGridTileLayoutData> CandidateTiles;
		for (const FGridTileLayoutData& Tile : Layout.Tiles)
		{
			if (!Tile.bWalkable || Tile.CellRole == EGridCellRole::Wall || Tile.CellRole == EGridCellRole::Start || Tile.GridCoord == Layout.ExitCoord)
			{
				continue;
			}

			if (!ReachableTiles.Contains(Tile.GridCoord))
			{
				continue;
			}

			if (!IsCandidateSafeFromStart(Tile.GridCoord, Settings, Layout))
			{
				continue;
			}

			CandidateTiles.Add(Tile);
		}

		CandidateTiles.Sort([](const FGridTileLayoutData& Left, const FGridTileLayoutData& Right)
		{
			if (Left.Depth != Right.Depth)
			{
				return Left.Depth > Right.Depth;
			}
			if (Left.GridCoord.Y != Right.GridCoord.Y)
			{
				return Left.GridCoord.Y < Right.GridCoord.Y;
			}
			return Left.GridCoord.X < Right.GridCoord.X;
		});

		for (int32 Index = 0; Index < CandidateTiles.Num(); ++Index)
		{
			const int32 SwapIndex = Stream.RandRange(Index, CandidateTiles.Num() - 1);
			CandidateTiles.Swap(Index, SwapIndex);
		}

		for (const FGridTileLayoutData& Tile : CandidateTiles)
		{
			if (Layout.EnemySpawnCandidates.Num() < Settings.EnemySpawnCount && Tile.CellRole == EGridCellRole::Room)
			{
				AddCandidate(Layout.EnemySpawnCandidates, Tile);
				continue;
			}

			if (Layout.EventSpawnCandidates.Num() < Settings.EventCandidateCount)
			{
				AddCandidate(Layout.EventSpawnCandidates, Tile);
				continue;
			}

			if (Layout.RewardSpawnCandidates.Num() < Settings.RewardCandidateCount && Tile.CellRole != EGridCellRole::Corridor)
			{
				AddCandidate(Layout.RewardSpawnCandidates, Tile);
			}

			if (Layout.EnemySpawnCandidates.Num() >= Settings.EnemySpawnCount
				&& Layout.EventSpawnCandidates.Num() >= Settings.EventCandidateCount
				&& Layout.RewardSpawnCandidates.Num() >= Settings.RewardCandidateCount)
			{
				break;
			}
		}
	}
}

bool ULSystemDungeonGenerator::GenerateDungeonLayout(const UDungeonGenerationSettings* Settings, FGeneratedDungeonLayout& OutLayout)
{
	OutLayout = FGeneratedDungeonLayout();
	if (!Settings)
	{
		UE_LOG(LogLSystemDungeonGenerator, Warning, TEXT("GenerateDungeonLayout failed: Settings is null."));
		return false;
	}

	if (Settings->Width < 8 || Settings->Height < 8 || Settings->MaxRoomCount <= 0)
	{
		UE_LOG(LogLSystemDungeonGenerator, Warning, TEXT("GenerateDungeonLayout failed: invalid settings."));
		return false;
	}

	for (int32 Attempt = 0; Attempt < Settings->MaxGenerationAttempts; ++Attempt)
	{
		// Retry attempts derive deterministic alternate streams from the configured seed.
		FRandomStream Stream(Settings->Seed + Attempt);
		OutLayout = FGeneratedDungeonLayout();
		OutLayout.Width = Settings->Width;
		OutLayout.Height = Settings->Height;
		OutLayout.Seed = Settings->Seed + Attempt;

		const bool bBuiltGraph = BuildRoomGraph(*Settings, Stream, OutLayout);
		const bool bRasterized = bBuiltGraph && RasterizeLayout(*Settings, Stream, OutLayout);
		if (bRasterized)
		{
			AssignTileTypes(*Settings, Stream, OutLayout);
			BuildSpawnCandidates(*Settings, Stream, OutLayout);
		}

		const bool bMatchesTargetRoomCount = Settings->TargetRoomCount <= 0
			|| OutLayout.Rooms.Num() == Settings->TargetRoomCount;
		if (bRasterized && bMatchesTargetRoomCount && ValidateConnectivity(OutLayout))
		{
			UE_LOG(LogLSystemDungeonGenerator, Log, TEXT("Generated dungeon layout: seed=%d rooms=%d connections=%d enemyCandidates=%d attempt=%d."),
				OutLayout.Seed, OutLayout.Rooms.Num(), OutLayout.Connections.Num(), OutLayout.EnemySpawnCandidates.Num(), Attempt + 1);
			return true;
		}

		UE_LOG(LogLSystemDungeonGenerator, Verbose, TEXT("Dungeon generation attempt failed: seed=%d attempt=%d rooms=%d targetRooms=%d."),
			Settings->Seed, Attempt + 1, OutLayout.Rooms.Num(), Settings->TargetRoomCount);
	}

	UE_LOG(LogLSystemDungeonGenerator, Warning, TEXT("GenerateDungeonLayout failed: all attempts produced invalid connectivity. seed=%d attempts=%d."),
		Settings->Seed, Settings->MaxGenerationAttempts);
	return false;
}

bool ULSystemDungeonGenerator::ValidateConnectivity(const FGeneratedDungeonLayout& Layout)
{
	TSet<FIntPoint> Visited;
	if (!BuildReachableTileSet(Layout, Visited))
	{
		return false;
	}

	int32 ReachableRoomCount = 0;
	for (const FLSystemDungeonRoomNode& Room : Layout.Rooms)
	{
		if (Visited.Contains(Room.Center))
		{
			++ReachableRoomCount;
		}
	}

	if (ReachableRoomCount != Layout.Rooms.Num())
	{
		UE_LOG(LogLSystemDungeonGenerator, Verbose, TEXT("Connectivity validation failed: reachableRooms=%d totalRooms=%d seed=%d."),
			ReachableRoomCount, Layout.Rooms.Num(), Layout.Seed);
		return false;
	}

	return !Layout.Rooms.IsEmpty();
}
