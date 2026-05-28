#include "Grid/GridManager.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Grid/GridSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridManager, Log, All);

AGridManager::AGridManager()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TileISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TileISM"));
	TileISM->SetupAttachment(SceneRoot);
	TileISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

	if (Tiles.IsEmpty())
	{
		GenerateGrid();
	}
}

void AGridManager::GenerateGrid()
{
	Tiles.Empty();

	if (TileISM)
	{
		// Regeneration starts from a clean visual layer to match the rebuilt Tiles map.
		TileISM->ClearInstances();
		if (GridSettings && GridSettings->TileMesh)
		{
			TileISM->SetStaticMesh(GridSettings->TileMesh);
		}
	}

	if (!GridSettings)
	{
		UE_LOG(LogGridManager, Warning, TEXT("GenerateGrid failed: GridSettings is not assigned."));
		return;
	}

	if (GridSettings->Width <= 0 || GridSettings->Height <= 0 || GridSettings->TileSize <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogGridManager, Warning, TEXT("GenerateGrid failed: invalid settings Width=%d Height=%d TileSize=%f."),
			GridSettings->Width, GridSettings->Height, GridSettings->TileSize);
		return;
	}

	Tiles.Reserve(GridSettings->Width * GridSettings->Height);

	for (int32 Y = 0; Y < GridSettings->Height; ++Y)
	{
		for (int32 X = 0; X < GridSettings->Width; ++X)
		{
			const FIntPoint Coord(X, Y);

			// Every tile starts with deterministic defaults before optional authored overrides.
			FTileData TileData;
			TileData.GridCoord = Coord;
			TileData.TileType = ETileType::Minimal;
			TileData.OccupantType = EGridOccupantType::Empty;
			TileData.OccupantActor.Reset();
			TileData.bWalkable = TileData.TileType != ETileType::Obstacle;
			TileData.bConvertible = true;

			// Apply authored prototype tile types after defaults so unlisted tiles remain Minimal.
			if (const FGridInitialTileOverride* TileOverride = GridSettings->InitialTileOverrides.FindByPredicate(
				[Coord](const FGridInitialTileOverride& Candidate)
				{
					return Candidate.GridCoord == Coord;
				}))
			{
				TileData.TileType = TileOverride->TileType;
				TileData.bWalkable = TileOverride->bWalkable && TileData.TileType != ETileType::Obstacle;
				TileData.bConvertible = TileOverride->bConvertible;
				TileData.OccupantType = TileData.TileType == ETileType::Obstacle ? EGridOccupantType::Obstacle : EGridOccupantType::Empty;
			}

			Tiles.Add(Coord, TileData);

			if (TileISM && GridSettings->TileMesh)
			{
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

				TileISM->AddInstance(FTransform(FRotator::ZeroRotator, GridToWorld(Coord), InstanceScale), true);
			}
		}
	}

	UE_LOG(LogGridManager, Log, TEXT("Generated grid: %d x %d (%d tiles)."),
		GridSettings->Width, GridSettings->Height, Tiles.Num());
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
	RefreshTileInstanceVisual(Coord, NewTileType);
	OnTileTypeChanged.Broadcast(Coord, NewTileType);
	return true;
}

void AGridManager::RefreshTileInstanceVisual_Implementation(const FIntPoint& Coord, ETileType NewTileType)
{
	// Blueprint GridManager variants can override this to recolor or swap one tile instance without rebuilding the grid.
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
