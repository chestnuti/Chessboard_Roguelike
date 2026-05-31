#include "PCG/DungeonRunManager.h"

#include "Core/TurnManager.h"
#include "Core/TurnStateTypes.h"
#include "Enemy/GridEnemyManager.h"
#include "Enemy/GridEnemyPawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Grid/GridManager.h"
#include "PCG/DungeonGenerationSettings.h"
#include "PCG/LSystemDungeonGenerator.h"
#include "Player/GridPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogDungeonRunManager, Log, All);

ADungeonRunManager::ADungeonRunManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ADungeonRunManager::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		GenerateAndInitializeRun();
	}
}

bool ADungeonRunManager::GenerateAndInitializeRun()
{
	if (bAutoFindReferences)
	{
		ResolveRuntimeReferences();
	}

	if (!DungeonGenerationSettings)
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("GenerateAndInitializeRun failed: DungeonGenerationSettings is not assigned."));
		return false;
	}

	if (PlayerPawn)
	{
		// PCG owns the start coordinate, so prevent the pawn from occupying its authored prototype start first.
		PlayerPawn->bAutoInitializeOnBeginPlay = false;
	}

	if (!ULSystemDungeonGenerator::GenerateDungeonLayout(DungeonGenerationSettings, LastGeneratedLayout))
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("GenerateAndInitializeRun failed: layout generation failed."));
		return false;
	}

	if (!ApplyGeneratedLayout())
	{
		return false;
	}

	if (bInitializePlayer && !InitializePlayerFromLayout())
	{
		return false;
	}

	if (bSpawnEnemies)
	{
		SpawnEnemiesFromLayout();
	}

	if (EnemyManager && GridManager && TurnManager && PlayerPawn)
	{
		// Rebuild after optional spawning so placed enemies and PCG-spawned enemies share the same manager list.
		EnemyManager->InitializeEnemyManager(GridManager, TurnManager, PlayerPawn);
		EnemyManager->RebuildEnemyList();
	}

	if (TurnManager)
	{
		TurnManager->SetTurnState(ETurnState::PlayerInput);
	}

	UE_LOG(LogDungeonRunManager, Log, TEXT("Dungeon run initialized: seed=%d rooms=%d enemyCandidates=%d."),
		LastGeneratedLayout.Seed, LastGeneratedLayout.Rooms.Num(), LastGeneratedLayout.EnemySpawnCandidates.Num());
	return true;
}

void ADungeonRunManager::ResolveRuntimeReferences()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!GridManager)
	{
		for (TActorIterator<AGridManager> It(World); It; ++It)
		{
			GridManager = *It;
			break;
		}
	}

	if (!TurnManager)
	{
		for (TActorIterator<ATurnManager> It(World); It; ++It)
		{
			TurnManager = *It;
			break;
		}
	}

	if (!PlayerPawn)
	{
		for (TActorIterator<AGridPawn> It(World); It; ++It)
		{
			PlayerPawn = *It;
			break;
		}
	}

	if (!EnemyManager)
	{
		for (TActorIterator<AGridEnemyManager> It(World); It; ++It)
		{
			EnemyManager = *It;
			break;
		}
	}
}

bool ADungeonRunManager::ApplyGeneratedLayout()
{
	if (!GridManager)
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("ApplyGeneratedLayout failed: GridManager is not assigned."));
		return false;
	}

	return GridManager->ApplyTileLayout(LastGeneratedLayout.Tiles, LastGeneratedLayout.Width, LastGeneratedLayout.Height);
}

bool ADungeonRunManager::InitializePlayerFromLayout()
{
	if (!PlayerPawn || !GridManager || !TurnManager)
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("InitializePlayerFromLayout failed: PlayerPawn, GridManager, or TurnManager is missing."));
		return false;
	}

	if (!GridManager->IsWalkable(LastGeneratedLayout.StartCoord))
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("InitializePlayerFromLayout failed: start coord (%d,%d) is not walkable."),
			LastGeneratedLayout.StartCoord.X, LastGeneratedLayout.StartCoord.Y);
		return false;
	}

	PlayerPawn->InitializeOnGrid(GridManager, TurnManager, LastGeneratedLayout.StartCoord);
	return true;
}

void ADungeonRunManager::SpawnEnemiesFromLayout()
{
	if (!DungeonGenerationSettings || !GridManager || !GetWorld())
	{
		UE_LOG(LogDungeonRunManager, Verbose, TEXT("SpawnEnemiesFromLayout skipped: DungeonGenerationSettings, GridManager, or World is missing."));
		return;
	}

	if (DungeonGenerationSettings->EnemySpawnPool.IsEmpty())
	{
		UE_LOG(LogDungeonRunManager, Verbose, TEXT("SpawnEnemiesFromLayout skipped: EnemySpawnPool is empty."));
		return;
	}

	// Use a layout-derived stream so enemy class selection remains deterministic for a generated map.
	FRandomStream SpawnStream(LastGeneratedLayout.Seed + 7919);
	const int32 EnemyCount = DungeonGenerationSettings->EnemySpawnCount;
	const int32 SpawnCount = FMath::Min(EnemyCount, LastGeneratedLayout.EnemySpawnCandidates.Num());

	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		const FDungeonSpawnCandidate& Candidate = LastGeneratedLayout.EnemySpawnCandidates[Index];
		if (!GridManager->IsWalkable(Candidate.Coord) || GridManager->IsOccupied(Candidate.Coord))
		{
			continue;
		}

		const TSubclassOf<AGridEnemyPawn> SelectedEnemyClass = SelectEnemyClassForCandidate(Candidate, SpawnStream);
		if (!SelectedEnemyClass)
		{
			UE_LOG(LogDungeonRunManager, Verbose, TEXT("SpawnEnemiesFromLayout skipped candidate (%d,%d): no valid enemy class for depth %d."),
				Candidate.Coord.X, Candidate.Coord.Y, Candidate.Depth);
			continue;
		}

		const FTransform SpawnTransform(FRotator::ZeroRotator, GridManager->GridToWorld(Candidate.Coord));
		// Deferred spawn lets us disable the enemy's automatic grid initialization before BeginPlay runs.
		AGridEnemyPawn* Enemy = GetWorld()->SpawnActorDeferred<AGridEnemyPawn>(
			SelectedEnemyClass,
			SpawnTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Enemy)
		{
			UE_LOG(LogDungeonRunManager, Warning, TEXT("SpawnEnemiesFromLayout failed at (%d,%d)."),
				Candidate.Coord.X, Candidate.Coord.Y);
			continue;
		}

		Enemy->bAutoInitializeOnBeginPlay = false;
		Enemy->FinishSpawning(SpawnTransform);
		Enemy->InitializeOnGrid(GridManager, Candidate.Coord);
		RegisterEnemyWithManager(Enemy);
	}
}

TSubclassOf<AGridEnemyPawn> ADungeonRunManager::SelectEnemyClassForCandidate(const FDungeonSpawnCandidate& Candidate, FRandomStream& Stream) const
{
	if (!DungeonGenerationSettings)
	{
		return nullptr;
	}

	// First pass gathers the total weight only from entries valid for this candidate depth.
	int32 TotalWeight = 0;
	for (const FDungeonEnemySpawnEntry& Entry : DungeonGenerationSettings->EnemySpawnPool)
	{
		if (!Entry.EnemyClass || Entry.Weight <= 0)
		{
			continue;
		}

		if (Candidate.Depth < Entry.MinDepth || Candidate.Depth > Entry.MaxDepth)
		{
			continue;
		}

		TotalWeight += Entry.Weight;
	}

	if (TotalWeight <= 0)
	{
		return nullptr;
	}

	int32 Roll = Stream.RandRange(1, TotalWeight);
	// Second pass consumes the weighted roll and returns the selected class.
	for (const FDungeonEnemySpawnEntry& Entry : DungeonGenerationSettings->EnemySpawnPool)
	{
		if (!Entry.EnemyClass || Entry.Weight <= 0)
		{
			continue;
		}

		if (Candidate.Depth < Entry.MinDepth || Candidate.Depth > Entry.MaxDepth)
		{
			continue;
		}

		Roll -= Entry.Weight;
		if (Roll <= 0)
		{
			return Entry.EnemyClass;
		}
	}

	return nullptr;
}

void ADungeonRunManager::RegisterEnemyWithManager(AGridEnemyPawn* Enemy) const
{
	if (EnemyManager && Enemy)
	{
		EnemyManager->RegisterEnemy(Enemy);
	}
}
