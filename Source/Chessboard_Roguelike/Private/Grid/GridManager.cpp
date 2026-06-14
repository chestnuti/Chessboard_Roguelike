#include "Grid/GridManager.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Grid/GridSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridManager, Log, All);

namespace
{
	constexpr int32 TileTypeCustomDataIndex = 0;
	constexpr int32 TileCoordXCustomDataIndex = 1;
	constexpr int32 TileCoordYCustomDataIndex = 2;
	constexpr int32 PlayerNextMoveCustomDataIndex = 3;
	constexpr int32 TileCustomDataFloatCount = 4;

	float TileTypeToCustomDataValue(ETileType TileType)
	{
		switch (TileType)
		{
		case ETileType::Construct:
			return 1.f;
		case ETileType::Acid:
			return 2.f;
		case ETileType::Obstacle:
			return 3.f;
		case ETileType::Minimal:
		default:
			return 0.f;
		}
	}

	int32 GetManhattanDistance(const FIntPoint& A, const FIntPoint& B)
	{
		return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
	}

	int32 GetPathScore(const TMap<FIntPoint, int32>& Scores, const FIntPoint& Coord)
	{
		if (const int32* Score = Scores.Find(Coord))
		{
			return *Score;
		}

		return MAX_int32 / 4;
	}

	void ReconstructPath(const TMap<FIntPoint, FIntPoint>& CameFrom, FIntPoint Current, TArray<FIntPoint>& OutPath)
	{
		OutPath.Reset();
		OutPath.Add(Current);

		while (const FIntPoint* Previous = CameFrom.Find(Current))
		{
			Current = *Previous;
			OutPath.Add(Current);
		}

		for (int32 Index = 0; Index < OutPath.Num() / 2; ++Index)
		{
			const int32 OppositeIndex = OutPath.Num() - Index - 1;
			const FIntPoint Temp = OutPath[Index];
			OutPath[Index] = OutPath[OppositeIndex];
			OutPath[OppositeIndex] = Temp;
		}
	}
}

AGridManager::AGridManager()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TileISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TileISM"));
	TileISM->SetupAttachment(SceneRoot);
	TileISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TileISM->NumCustomDataFloats = TileCustomDataFloatCount;
}

void AGridManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Rebuild editor preview whenever GridSettings or actor transform changes.
	GenerateGrid();
}

void AGridManager::BeginPlay()
{
	Super::BeginPlay();

	if (Tiles.IsEmpty() || TileInstanceIndices.Num() != Tiles.Num())
	{
		GenerateGrid();
	}
}

void AGridManager::GenerateGrid()
{
	ClearGridState();

	if (!HasValidGridSettings())
	{
		return;
	}

	CurrentWidth = GridSettings->Width;
	CurrentHeight = GridSettings->Height;

	TArray<FGridTileLayoutData> TileLayout;
	BuildDefaultTileLayout(TileLayout);
	ApplyInitialTileOverrides(TileLayout);
	ApplyTileLayoutInternal(TileLayout);
	RebuildTileVisuals();

	UE_LOG(LogGridManager, Log, TEXT("Generated grid: %d x %d (%d tiles)."),
		CurrentWidth, CurrentHeight, Tiles.Num());
}

bool AGridManager::ApplyTileLayout(const TArray<FGridTileLayoutData>& TileLayout, int32 LayoutWidth, int32 LayoutHeight)
{
	ClearGridState();

	if (LayoutWidth <= 0 || LayoutHeight <= 0)
	{
		UE_LOG(LogGridManager, Warning, TEXT("ApplyTileLayout failed: invalid layout dimensions Width=%d Height=%d."),
			LayoutWidth, LayoutHeight);
		return false;
	}

	if (!GridSettings || GridSettings->TileSize <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogGridManager, Warning, TEXT("ApplyTileLayout failed: GridSettings with a valid TileSize is required."));
		return false;
	}

	if (TileLayout.IsEmpty())
	{
		UE_LOG(LogGridManager, Warning, TEXT("ApplyTileLayout failed: TileLayout is empty."));
		return false;
	}

	CurrentWidth = LayoutWidth;
	CurrentHeight = LayoutHeight;
	ApplyTileLayoutInternal(TileLayout);
	RebuildTileVisuals();

	UE_LOG(LogGridManager, Log, TEXT("Applied tile layout: %d x %d (%d tiles)."),
		CurrentWidth, CurrentHeight, Tiles.Num());
	return true;
}

bool AGridManager::HasValidGridSettings() const
{
	if (!GridSettings)
	{
		UE_LOG(LogGridManager, Warning, TEXT("GenerateGrid failed: GridSettings is not assigned."));
		return false;
	}

	if (GridSettings->Width <= 0 || GridSettings->Height <= 0 || GridSettings->TileSize <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogGridManager, Warning, TEXT("GenerateGrid failed: invalid settings Width=%d Height=%d TileSize=%f."),
			GridSettings->Width, GridSettings->Height, GridSettings->TileSize);
		return false;
	}

	return true;
}

void AGridManager::ClearGridState()
{
	Tiles.Empty();
	TileInstanceIndices.Empty();
	PlayerNextMoveTileCoords.Empty();
	CurrentWidth = 0;
	CurrentHeight = 0;

	if (TileISM)
	{
		TileISM->ClearInstances();
		TileISM->NumCustomDataFloats = TileCustomDataFloatCount;
		if (GridSettings && GridSettings->TileMesh)
		{
			TileISM->SetStaticMesh(GridSettings->TileMesh);
		}
	}
}

void AGridManager::BuildDefaultTileLayout(TArray<FGridTileLayoutData>& OutTileLayout) const
{
	OutTileLayout.Reset();
	if (!GridSettings)
	{
		return;
	}

	OutTileLayout.Reserve(GridSettings->Width * GridSettings->Height);
	for (int32 Y = 0; Y < GridSettings->Height; ++Y)
	{
		for (int32 X = 0; X < GridSettings->Width; ++X)
		{
			FGridTileLayoutData LayoutData;
			LayoutData.GridCoord = FIntPoint(X, Y);
			LayoutData.TileType = ETileType::Minimal;
			LayoutData.CellRole = EGridCellRole::Open;
			LayoutData.bWalkable = true;
			LayoutData.bConvertible = true;
			LayoutData.RegionId = INDEX_NONE;
			LayoutData.Depth = 0;
			OutTileLayout.Add(LayoutData);
		}
	}
}

void AGridManager::ApplyInitialTileOverrides(TArray<FGridTileLayoutData>& TileLayout) const
{
	if (!GridSettings)
	{
		return;
	}

	for (const FGridInitialTileOverride& TileOverride : GridSettings->InitialTileOverrides)
	{
		if (TileOverride.GridCoord.X < 0 || TileOverride.GridCoord.Y < 0
			|| TileOverride.GridCoord.X >= GridSettings->Width || TileOverride.GridCoord.Y >= GridSettings->Height)
		{
			UE_LOG(LogGridManager, Warning, TEXT("Initial tile override ignored: invalid coord (%d,%d)."),
				TileOverride.GridCoord.X, TileOverride.GridCoord.Y);
			continue;
		}

		const int32 TileIndex = TileOverride.GridCoord.Y * GridSettings->Width + TileOverride.GridCoord.X;
		if (!TileLayout.IsValidIndex(TileIndex))
		{
			continue;
		}

		FGridTileLayoutData& LayoutData = TileLayout[TileIndex];
		LayoutData.TileType = TileOverride.TileType;
		LayoutData.CellRole = TileOverride.CellRole;
		LayoutData.bWalkable = TileOverride.bWalkable;
		LayoutData.bConvertible = TileOverride.bConvertible;
		LayoutData.RegionId = TileOverride.RegionId;
		LayoutData.Depth = TileOverride.Depth;
	}
}

void AGridManager::ApplyTileLayoutInternal(const TArray<FGridTileLayoutData>& TileLayout)
{
	Tiles.Reserve(TileLayout.Num());
	for (const FGridTileLayoutData& LayoutData : TileLayout)
	{
		if (LayoutData.GridCoord.X < 0 || LayoutData.GridCoord.Y < 0
			|| LayoutData.GridCoord.X >= CurrentWidth || LayoutData.GridCoord.Y >= CurrentHeight)
		{
			UE_LOG(LogGridManager, Warning, TEXT("Tile layout entry ignored: coord (%d,%d) outside %d x %d."),
				LayoutData.GridCoord.X, LayoutData.GridCoord.Y, CurrentWidth, CurrentHeight);
			continue;
		}

		Tiles.Add(LayoutData.GridCoord, MakeTileDataFromLayout(LayoutData));
	}
}

FTileData AGridManager::MakeTileDataFromLayout(const FGridTileLayoutData& LayoutData) const
{
	FTileData TileData;
	TileData.GridCoord = LayoutData.GridCoord;
	TileData.TileType = LayoutData.TileType;
	TileData.CellRole = LayoutData.CellRole;
	TileData.RegionId = LayoutData.RegionId;
	TileData.Depth = LayoutData.Depth;
	TileData.OccupantActor.Reset();
	TileData.bConvertible = LayoutData.bConvertible;

	const bool bBlockingCell = LayoutData.TileType == ETileType::Obstacle || LayoutData.CellRole == EGridCellRole::Wall;
	TileData.bWalkable = LayoutData.bWalkable && !bBlockingCell;
	TileData.OccupantType = TileData.bWalkable ? EGridOccupantType::Empty : EGridOccupantType::Obstacle;
	return TileData;
}

void AGridManager::RebuildTileVisuals()
{
	if (!TileISM || !GridSettings || !GridSettings->TileMesh)
	{
		return;
	}

	TileISM->ClearInstances();
	TileInstanceIndices.Empty();
	TileISM->NumCustomDataFloats = TileCustomDataFloatCount;
	TileISM->SetStaticMesh(GridSettings->TileMesh);

	if (CurrentWidth > 0 && CurrentHeight > 0)
	{
		for (int32 Y = 0; Y < CurrentHeight; ++Y)
		{
			for (int32 X = 0; X < CurrentWidth; ++X)
			{
				if (const FTileData* TileData = Tiles.Find(FIntPoint(X, Y)))
				{
					AddTileVisualInstance(*TileData);
				}
			}
		}
		return;
	}

	for (const TPair<FIntPoint, FTileData>& TilePair : Tiles)
	{
		AddTileVisualInstance(TilePair.Value);
	}
}

void AGridManager::AddTileVisualInstance(const FTileData& TileData)
{
	if (!TileISM || !GridSettings || !GridSettings->TileMesh)
	{
		return;
	}

	FVector InstanceScale = FVector::OneVector;
	const FVector MeshSize = GridSettings->TileMesh->GetBounds().BoxExtent * 2.f;
	// Scale the visual mesh to TileSize so changing settings affects both logic and display.
	if (MeshSize.X > KINDA_SMALL_NUMBER)
	{
		InstanceScale.X = GridSettings->TileSize / MeshSize.X;
	}
	if (MeshSize.Y > KINDA_SMALL_NUMBER)
	{
		InstanceScale.Y = GridSettings->TileSize / MeshSize.Y;
	}

	const int32 InstanceIndex = TileISM->AddInstance(FTransform(FRotator::ZeroRotator, GridToWorld(TileData.GridCoord), InstanceScale), true);
	TileInstanceIndices.Add(TileData.GridCoord, InstanceIndex);

	// Materials can read PerInstanceCustomData[0..3] as TileType, GridCoord.X, GridCoord.Y, bPlayerCanMoveNext.
	ApplyTileInstanceCustomData(TileData.GridCoord, TileData.TileType);
}

bool AGridManager::IsValidCoord(FIntPoint Coord) const
{
	return Tiles.Contains(Coord);
}

bool AGridManager::IsWalkable(FIntPoint Coord) const
{
	const FTileData* TileData = Tiles.Find(Coord);
	return TileData && TileData->bWalkable && TileData->TileType != ETileType::Obstacle;
}

bool AGridManager::IsOccupied(FIntPoint Coord) const
{
	const FTileData* TileData = Tiles.Find(Coord);
	return TileData && TileData->OccupantType != EGridOccupantType::Empty;
}

bool AGridManager::GetTileData(const FIntPoint& Coord, FTileData& OutTileData) const
{
	const FTileData* TileData = Tiles.Find(Coord);
	if (!TileData)
	{
		return false;
	}

	OutTileData = *TileData;
	return true;
}

bool AGridManager::SetTileType(const FIntPoint& Coord, ETileType NewTileType)
{
	FTileData* TileData = Tiles.Find(Coord);
	if (!TileData)
	{
		UE_LOG(LogGridManager, Warning, TEXT("SetTileType failed: invalid coord (%d,%d)."), Coord.X, Coord.Y);
		return false;
	}

	if (TileData->TileType == NewTileType)
	{
		return true;
	}

	// Change only the terrain type; movement occupancy and walkability are managed by the grid movement layer.
	TileData->TileType = NewTileType;
	ApplyTileInstanceCustomData(Coord, NewTileType);
	RefreshTileInstanceVisual(Coord, NewTileType);
	OnTileTypeChanged.Broadcast(Coord, NewTileType);
	return true;
}

void AGridManager::RefreshTileInstanceVisual_Implementation(const FIntPoint& Coord, ETileType NewTileType)
{
	// Blueprint GridManager variants can override this for extra effects; core custom data is applied before this hook.
}

void AGridManager::ApplyTileInstanceCustomData(const FIntPoint& Coord, ETileType NewTileType)
{
	if (!TileISM)
	{
		return;
	}

	const int32* InstanceIndex = TileInstanceIndices.Find(Coord);
	if (!InstanceIndex)
	{
		UE_LOG(LogGridManager, Warning, TEXT("ApplyTileInstanceCustomData failed: coord (%d,%d) has no instance index."),
			Coord.X, Coord.Y);
		return;
	}

	const float CustomDataValue = TileTypeToCustomDataValue(NewTileType);
	const float PlayerNextMoveValue = PlayerNextMoveTileCoords.Contains(Coord) ? 1.f : 0.f;
	const bool bWroteTileType = TileISM->SetCustomDataValue(*InstanceIndex, TileTypeCustomDataIndex, CustomDataValue, false);
	const bool bWroteGridX = TileISM->SetCustomDataValue(*InstanceIndex, TileCoordXCustomDataIndex, static_cast<float>(Coord.X), false);
	const bool bWroteGridY = TileISM->SetCustomDataValue(*InstanceIndex, TileCoordYCustomDataIndex, static_cast<float>(Coord.Y), false);
	const bool bWrotePlayerNextMove = TileISM->SetCustomDataValue(*InstanceIndex, PlayerNextMoveCustomDataIndex, PlayerNextMoveValue, true);
	if (!bWroteTileType || !bWroteGridX || !bWroteGridY || !bWrotePlayerNextMove)
	{
		UE_LOG(LogGridManager, Warning, TEXT("ApplyTileInstanceCustomData failed: InstanceIndex=%d TileType=%f Coord=(%d,%d) NextMove=%f NumCustomDataFloats=%d."),
			*InstanceIndex, CustomDataValue, Coord.X, Coord.Y, PlayerNextMoveValue, TileISM->NumCustomDataFloats);
	}
}

void AGridManager::SetTilePlayerNextMoveCustomData(const FIntPoint& Coord, bool bCanMoveNext)
{
	if (!TileISM)
	{
		return;
	}

	const int32* InstanceIndex = TileInstanceIndices.Find(Coord);
	if (!InstanceIndex)
	{
		UE_LOG(LogGridManager, Warning, TEXT("SetTilePlayerNextMoveCustomData failed: coord (%d,%d) has no instance index."),
			Coord.X, Coord.Y);
		return;
	}

	const float CustomDataValue = bCanMoveNext ? 1.f : 0.f;
	if (!TileISM->SetCustomDataValue(*InstanceIndex, PlayerNextMoveCustomDataIndex, CustomDataValue, true))
	{
		UE_LOG(LogGridManager, Warning, TEXT("SetTilePlayerNextMoveCustomData failed: InstanceIndex=%d Value=%f NumCustomDataFloats=%d."),
			*InstanceIndex, CustomDataValue, TileISM->NumCustomDataFloats);
	}
}

bool AGridManager::GetTileInstanceIndex(const FIntPoint& Coord, int32& OutInstanceIndex) const
{
	if (const int32* InstanceIndex = TileInstanceIndices.Find(Coord))
	{
		OutInstanceIndex = *InstanceIndex;
		return true;
	}

	OutInstanceIndex = INDEX_NONE;
	return false;
}

void AGridManager::ClearPlayerNextMoveTiles()
{
	if (PlayerNextMoveTileCoords.IsEmpty())
	{
		return;
	}

	const TSet<FIntPoint> PreviousMoveCoords = PlayerNextMoveTileCoords;
	PlayerNextMoveTileCoords.Empty();

	for (const FIntPoint& Coord : PreviousMoveCoords)
	{
		SetTilePlayerNextMoveCustomData(Coord, false);
	}
}

void AGridManager::SetPlayerNextMoveTiles(const TArray<FIntPoint>& MoveCoords)
{
	ClearPlayerNextMoveTiles();

	for (const FIntPoint& Coord : MoveCoords)
	{
		if (!Tiles.Contains(Coord))
		{
			continue;
		}

		PlayerNextMoveTileCoords.Add(Coord);
		SetTilePlayerNextMoveCustomData(Coord, true);
	}
}

bool AGridManager::IsTileConvertible(const FIntPoint& Coord) const
{
	const FTileData* TileData = Tiles.Find(Coord);
	return TileData && TileData->bConvertible && TileData->TileType != ETileType::Obstacle;
}

FVector AGridManager::GridToWorld(FIntPoint Coord) const
{
	const float TileSize = GridSettings ? GridSettings->TileSize : 200.f;
	const FVector Origin = GridSettings ? GridSettings->Origin : FVector::ZeroVector;
	// World position is presentation only; the authoritative location is still Coord.
	return Origin + FVector(Coord.X * TileSize, Coord.Y * TileSize, 0.f);
}

FIntPoint AGridManager::WorldToGrid(FVector WorldLocation) const
{
	if (!GridSettings || GridSettings->TileSize <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogGridManager, Warning, TEXT("WorldToGrid used without valid GridSettings."));
		return FIntPoint::ZeroValue;
	}

	const FVector LocalLocation = WorldLocation - GridSettings->Origin;
	return FIntPoint(
		FMath::RoundToInt(LocalLocation.X / GridSettings->TileSize),
		FMath::RoundToInt(LocalLocation.Y / GridSettings->TileSize));
}

bool AGridManager::FindPathAStar(FIntPoint StartCoord, FIntPoint GoalCoord, TArray<FIntPoint>& OutPath, bool bAllowOccupiedGoal) const
{
	OutPath.Reset();

	const FTileData* StartTile = Tiles.Find(StartCoord);
	const FTileData* GoalTile = Tiles.Find(GoalCoord);
	if (!StartTile || !GoalTile)
	{
		return false;
	}

	if (!StartTile->bWalkable || StartTile->TileType == ETileType::Obstacle
		|| !GoalTile->bWalkable || GoalTile->TileType == ETileType::Obstacle)
	{
		return false;
	}

	if (GoalTile->OccupantType != EGridOccupantType::Empty && !bAllowOccupiedGoal)
	{
		return false;
	}

	if (StartCoord == GoalCoord)
	{
		OutPath.Add(StartCoord);
		return true;
	}

	static const FIntPoint NeighborOffsets[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	TArray<FIntPoint> OpenSet;
	TSet<FIntPoint> ClosedSet;
	TMap<FIntPoint, FIntPoint> CameFrom;
	TMap<FIntPoint, int32> GScore;
	TMap<FIntPoint, int32> FScore;

	OpenSet.Add(StartCoord);
	GScore.Add(StartCoord, 0);
	FScore.Add(StartCoord, GetManhattanDistance(StartCoord, GoalCoord));

	while (!OpenSet.IsEmpty())
	{
		int32 BestOpenIndex = 0;
		int32 BestFScore = GetPathScore(FScore, OpenSet[0]);
		int32 BestHeuristic = GetManhattanDistance(OpenSet[0], GoalCoord);

		for (int32 Index = 1; Index < OpenSet.Num(); ++Index)
		{
			const FIntPoint& Candidate = OpenSet[Index];
			const int32 CandidateFScore = GetPathScore(FScore, Candidate);
			const int32 CandidateHeuristic = GetManhattanDistance(Candidate, GoalCoord);
			if (CandidateFScore < BestFScore
				|| (CandidateFScore == BestFScore && CandidateHeuristic < BestHeuristic))
			{
				BestOpenIndex = Index;
				BestFScore = CandidateFScore;
				BestHeuristic = CandidateHeuristic;
			}
		}

		const FIntPoint Current = OpenSet[BestOpenIndex];
		if (Current == GoalCoord)
		{
			ReconstructPath(CameFrom, Current, OutPath);
			return true;
		}

		OpenSet.RemoveAtSwap(BestOpenIndex);
		ClosedSet.Add(Current);

		for (const FIntPoint& Offset : NeighborOffsets)
		{
			const FIntPoint Neighbor = Current + Offset;
			if (ClosedSet.Contains(Neighbor))
			{
				continue;
			}

			const FTileData* NeighborTile = Tiles.Find(Neighbor);
			if (!NeighborTile || !NeighborTile->bWalkable || NeighborTile->TileType == ETileType::Obstacle)
			{
				continue;
			}

			const bool bIsGoal = Neighbor == GoalCoord;
			if (NeighborTile->OccupantType != EGridOccupantType::Empty && !(bIsGoal && bAllowOccupiedGoal))
			{
				continue;
			}

			const int32 TentativeGScore = GetPathScore(GScore, Current) + 1;
			if (TentativeGScore >= GetPathScore(GScore, Neighbor))
			{
				continue;
			}

			CameFrom.Add(Neighbor, Current);
			GScore.Add(Neighbor, TentativeGScore);
			FScore.Add(Neighbor, TentativeGScore + GetManhattanDistance(Neighbor, GoalCoord));

			OpenSet.AddUnique(Neighbor);
		}
	}

	return false;
}

bool AGridManager::TryOccupyTile(FIntPoint Coord, AActor* Occupant, EGridOccupantType OccupantType)
{
	// Placement is allowed only on existing, walkable, currently empty tiles.
	if (!Occupant || OccupantType == EGridOccupantType::Empty)
	{
		UE_LOG(LogGridManager, Warning, TEXT("TryOccupyTile failed at (%d,%d): invalid occupant."), Coord.X, Coord.Y);
		return false;
	}

	FTileData* TileData = Tiles.Find(Coord);
	if (!TileData)
	{
		UE_LOG(LogGridManager, Warning, TEXT("TryOccupyTile failed: invalid coord (%d,%d)."), Coord.X, Coord.Y);
		return false;
	}

	if (!TileData->bWalkable || TileData->TileType == ETileType::Obstacle)
	{
		UE_LOG(LogGridManager, Verbose, TEXT("TryOccupyTile blocked: coord (%d,%d) is not walkable."), Coord.X, Coord.Y);
		return false;
	}

	if (TileData->OccupantType != EGridOccupantType::Empty)
	{
		UE_LOG(LogGridManager, Verbose, TEXT("TryOccupyTile blocked: coord (%d,%d) is occupied."), Coord.X, Coord.Y);
		return false;
	}

	TileData->OccupantType = OccupantType;
	TileData->OccupantActor = Occupant;
	return true;
}

void AGridManager::ClearOccupant(FIntPoint Coord)
{
	FTileData* TileData = Tiles.Find(Coord);
	if (!TileData)
	{
		UE_LOG(LogGridManager, Warning, TEXT("ClearOccupant ignored: invalid coord (%d,%d)."), Coord.X, Coord.Y);
		return;
	}

	TileData->OccupantType = EGridOccupantType::Empty;
	TileData->OccupantActor.Reset();
}

bool AGridManager::RequestMove(AActor* Unit, FIntPoint FromCoord, FIntPoint ToCoord)
{
	// Validate everything up front so failed moves leave the grid unchanged.
	if (!Unit)
	{
		UE_LOG(LogGridManager, Warning, TEXT("RequestMove failed: Unit is null."));
		return false;
	}

	FTileData* SourceTile = Tiles.Find(FromCoord);
	const FTileData* TargetTile = Tiles.Find(ToCoord);
	if (!SourceTile || !TargetTile)
	{
		UE_LOG(LogGridManager, Verbose, TEXT("RequestMove failed: invalid move (%d,%d) -> (%d,%d)."),
			FromCoord.X, FromCoord.Y, ToCoord.X, ToCoord.Y);
		return false;
	}

	if (SourceTile->OccupantActor.Get() != Unit)
	{
		UE_LOG(LogGridManager, Warning, TEXT("RequestMove failed: source (%d,%d) is not occupied by %s."),
			FromCoord.X, FromCoord.Y, *GetNameSafe(Unit));
		return false;
	}

	const EGridOccupantType MovingOccupantType = SourceTile->OccupantType;
	if (MovingOccupantType == EGridOccupantType::Empty)
	{
		UE_LOG(LogGridManager, Warning, TEXT("RequestMove failed: source (%d,%d) is empty."), FromCoord.X, FromCoord.Y);
		return false;
	}

	if (!TargetTile->bWalkable || TargetTile->TileType == ETileType::Obstacle)
	{
		UE_LOG(LogGridManager, Verbose, TEXT("RequestMove failed: target (%d,%d) is not walkable."), ToCoord.X, ToCoord.Y);
		return false;
	}

	if (TargetTile->OccupantType != EGridOccupantType::Empty)
	{
		UE_LOG(LogGridManager, Verbose, TEXT("RequestMove failed: target (%d,%d) is occupied."), ToCoord.X, ToCoord.Y);
		return false;
	}

	ClearOccupant(FromCoord);

	FTileData* MutableTargetTile = Tiles.Find(ToCoord);
	check(MutableTargetTile);
	// Preserve the moving occupant type for later enemy/unit reuse while the pawn remains caller-owned.
	MutableTargetTile->OccupantType = MovingOccupantType;
	MutableTargetTile->OccupantActor = Unit;
	return true;
}
