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
#include "Pickup/GridPickupActor.h"
#include "Pickup/GridPickupManager.h"
#include "Player/GridPawn.h"
#include "Tutorial/TutorialLevelSet.h"

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

	if (PlayerPawn)
	{
		// PCG owns the start coordinate, so prevent the pawn from occupying its authored prototype start first.
		PlayerPawn->bAutoInitializeOnBeginPlay = false;
	}

	const bool bUseTutorialFixed = GenerationMode == EDungeonRunGenerationMode::TutorialFixed;
	if (bUseTutorialFixed)
	{
		if (!GenerateTutorialRun())
		{
			return false;
		}
	}
	else
	{
		if (!DungeonGenerationSettings)
		{
			UE_LOG(LogDungeonRunManager, Warning, TEXT("GenerateAndInitializeRun failed: DungeonGenerationSettings is not assigned."));
			return false;
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

		if (bSpawnPickups)
		{
			SpawnPickupsFromLayout();
		}
	}

	if (EnemyManager && GridManager && TurnManager && PlayerPawn)
	{
		// Rebuild after optional spawning so placed enemies and runtime-spawned enemies share the same manager list.
		EnemyManager->InitializeEnemyManager(GridManager, TurnManager, PlayerPawn);
		EnemyManager->RebuildEnemyList();
	}

	if (TurnManager)
	{
		TurnManager->SetTurnState(ETurnState::PlayerInput);
	}
	if (PlayerPawn)
	{
		PlayerPawn->RefreshPlayerNextMoveTiles();
	}

	UE_LOG(LogDungeonRunManager, Log, TEXT("Dungeon run initialized: mode=%s seed=%d rooms=%d enemyCandidates=%d rewardCandidates=%d."),
		bUseTutorialFixed ? TEXT("TutorialFixed") : TEXT("Procedural"),
		LastGeneratedLayout.Seed,
		LastGeneratedLayout.Rooms.Num(),
		LastGeneratedLayout.EnemySpawnCandidates.Num(),
		LastGeneratedLayout.RewardSpawnCandidates.Num());
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

	if (!PickupManager)
	{
		for (TActorIterator<AGridPickupManager> It(World); It; ++It)
		{
			PickupManager = *It;
			break;
		}
	}
}

bool ADungeonRunManager::GenerateTutorialRun()
{
	const UTutorialLevelSet* ResolvedLevelSet = TutorialLevelSet ? TutorialLevelSet.Get() : GetDefault<UTutorialLevelSet>();
	if (!ResolvedLevelSet)
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("GenerateTutorialRun failed: TutorialLevelSet is missing."));
		return false;
	}

	if (!ResolvedLevelSet->TutorialLevels.IsValidIndex(TutorialLevelIndex))
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("GenerateTutorialRun failed: TutorialLevelIndex %d is outside level count %d."),
			TutorialLevelIndex, ResolvedLevelSet->TutorialLevels.Num());
		return false;
	}

	const FTutorialLevelDefinition& TutorialLevel = ResolvedLevelSet->TutorialLevels[TutorialLevelIndex];
	if (!ApplyTutorialLevel(TutorialLevel))
	{
		return false;
	}

	if (bInitializePlayer && !InitializePlayerAtCoord(TutorialLevel.StartCoord))
	{
		return false;
	}

	SpawnTutorialEnemies(TutorialLevel);
	return true;
}

bool ADungeonRunManager::ApplyTutorialLevel(const FTutorialLevelDefinition& TutorialLevel)
{
	if (!GridManager)
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("ApplyTutorialLevel failed: GridManager is not assigned."));
		return false;
	}

	if (TutorialLevel.Width != 10 || TutorialLevel.Height != 10)
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("ApplyTutorialLevel failed: %s is %d x %d, expected 10 x 10."),
			*TutorialLevel.LevelId.ToString(), TutorialLevel.Width, TutorialLevel.Height);
		return false;
	}

	const int32 ExpectedTileCount = TutorialLevel.Width * TutorialLevel.Height;
	if (TutorialLevel.Tiles.Num() != ExpectedTileCount)
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("ApplyTutorialLevel failed: %s has %d tiles, expected %d."),
			*TutorialLevel.LevelId.ToString(), TutorialLevel.Tiles.Num(), ExpectedTileCount);
		return false;
	}

	LastGeneratedLayout = FGeneratedDungeonLayout();
	LastGeneratedLayout.Width = TutorialLevel.Width;
	LastGeneratedLayout.Height = TutorialLevel.Height;
	LastGeneratedLayout.Seed = TutorialLevelIndex;
	LastGeneratedLayout.Tiles = TutorialLevel.Tiles;
	LastGeneratedLayout.StartCoord = TutorialLevel.StartCoord;
	LastGeneratedLayout.ExitCoord = TutorialLevel.ExitCoord;

	return GridManager->ApplyTileLayout(LastGeneratedLayout.Tiles, LastGeneratedLayout.Width, LastGeneratedLayout.Height);
}

bool ADungeonRunManager::InitializePlayerAtCoord(FIntPoint StartCoord)
{
	if (!PlayerPawn || !GridManager || !TurnManager)
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("InitializePlayerAtCoord failed: PlayerPawn, GridManager, or TurnManager is missing."));
		return false;
	}

	if (!GridManager->IsWalkable(StartCoord))
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("InitializePlayerAtCoord failed: start coord (%d,%d) is not walkable."),
			StartCoord.X, StartCoord.Y);
		return false;
	}

	PlayerPawn->InitializeOnGrid(GridManager, TurnManager, StartCoord);
	return true;
}

void ADungeonRunManager::SpawnTutorialEnemies(const FTutorialLevelDefinition& TutorialLevel)
{
	if (!GridManager || !GetWorld())
	{
		UE_LOG(LogDungeonRunManager, Verbose, TEXT("SpawnTutorialEnemies skipped: GridManager or World is missing."));
		return;
	}

	for (const FTutorialEnemySpawnData& EnemyData : TutorialLevel.Enemies)
	{
		if (!GridManager->IsWalkable(EnemyData.Coord) || GridManager->IsOccupied(EnemyData.Coord))
		{
			UE_LOG(LogDungeonRunManager, Warning, TEXT("SpawnTutorialEnemies skipped %s enemy at (%d,%d): tile is blocked or occupied."),
				*TutorialLevel.LevelId.ToString(), EnemyData.Coord.X, EnemyData.Coord.Y);
			continue;
		}

		TSubclassOf<AGridEnemyPawn> EnemyClass = EnemyData.EnemyClass;
		if (!EnemyClass)
		{
			EnemyClass = AGridEnemyPawn::StaticClass();
		}
		const FTransform SpawnTransform(FRotator::ZeroRotator, GridManager->GridToWorld(EnemyData.Coord));
		AGridEnemyPawn* Enemy = GetWorld()->SpawnActorDeferred<AGridEnemyPawn>(
			EnemyClass,
			SpawnTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Enemy)
		{
			UE_LOG(LogDungeonRunManager, Warning, TEXT("SpawnTutorialEnemies failed at (%d,%d)."),
				EnemyData.Coord.X, EnemyData.Coord.Y);
			continue;
		}

		Enemy->bAutoInitializeOnBeginPlay = false;
		Enemy->Faction = EnemyData.Faction;
		Enemy->BehaviorType = EnemyData.BehaviorType;
		Enemy->KillThreshold = FMath::Max(1, EnemyData.KillThreshold);
		Enemy->MaxRangedAttackDistance = FMath::Max(0, EnemyData.MaxRangedAttackDistance);
		Enemy->FinishSpawning(SpawnTransform);
		Enemy->InitializeOnGrid(GridManager, EnemyData.Coord);
		RegisterEnemyWithManager(Enemy);
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
	return InitializePlayerAtCoord(LastGeneratedLayout.StartCoord);
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

		const FDungeonEnemySpawnEntry* SelectedSpawnEntry = SelectEnemySpawnEntryForCandidate(Candidate, SpawnStream);
		if (!SelectedSpawnEntry)
		{
			UE_LOG(LogDungeonRunManager, Verbose, TEXT("SpawnEnemiesFromLayout skipped candidate (%d,%d): no valid enemy class for depth %d."),
				Candidate.Coord.X, Candidate.Coord.Y, Candidate.Depth);
			continue;
		}

		const FTransform SpawnTransform(FRotator::ZeroRotator, GridManager->GridToWorld(Candidate.Coord));
		// Deferred spawn lets us disable the enemy's automatic grid initialization before BeginPlay runs.
		AGridEnemyPawn* Enemy = GetWorld()->SpawnActorDeferred<AGridEnemyPawn>(
			SelectedSpawnEntry->EnemyClass,
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
		Enemy->KillThreshold = CalculateEnemyKillThresholdForCandidate(Enemy, Candidate, *SelectedSpawnEntry);
		Enemy->InitializeOnGrid(GridManager, Candidate.Coord);
		RegisterEnemyWithManager(Enemy);
	}
}

void ADungeonRunManager::SpawnPickupsFromLayout()
{
	if (!DungeonGenerationSettings || !GridManager || !GetWorld())
	{
		UE_LOG(LogDungeonRunManager, Verbose, TEXT("SpawnPickupsFromLayout skipped: DungeonGenerationSettings, GridManager, or World is missing."));
		return;
	}

	AGridPickupManager* ResolvedPickupManager = EnsurePickupManager();
	if (!ResolvedPickupManager)
	{
		UE_LOG(LogDungeonRunManager, Warning, TEXT("SpawnPickupsFromLayout skipped: PickupManager is missing and could not be spawned."));
		return;
	}

	ResolvedPickupManager->ClearAllPickups();

	if (DungeonGenerationSettings->PickupSpawnPool.IsEmpty())
	{
		UE_LOG(LogDungeonRunManager, Verbose, TEXT("SpawnPickupsFromLayout skipped: PickupSpawnPool is empty."));
		return;
	}

	FRandomStream SpawnStream(LastGeneratedLayout.Seed + 15485863);
	const int32 TargetSpawnCount = FMath::Min(DungeonGenerationSettings->PickupSpawnCount, LastGeneratedLayout.RewardSpawnCandidates.Num());
	int32 SpawnedCount = 0;

	for (int32 Index = 0; Index < LastGeneratedLayout.RewardSpawnCandidates.Num() && SpawnedCount < TargetSpawnCount; ++Index)
	{
		const FDungeonSpawnCandidate& Candidate = LastGeneratedLayout.RewardSpawnCandidates[Index];
		if (!GridManager->IsWalkable(Candidate.Coord) || GridManager->IsOccupied(Candidate.Coord))
		{
			continue;
		}

		if (ResolvedPickupManager->GetPickupAt(Candidate.Coord))
		{
			continue;
		}

		const FDungeonPickupSpawnEntry* SelectedSpawnEntry = SelectPickupSpawnEntryForCandidate(Candidate, SpawnStream);
		if (!SelectedSpawnEntry)
		{
			UE_LOG(LogDungeonRunManager, Verbose, TEXT("SpawnPickupsFromLayout skipped candidate (%d,%d): no valid pickup class for depth %d."),
				Candidate.Coord.X, Candidate.Coord.Y, Candidate.Depth);
			continue;
		}

		const FTransform SpawnTransform(FRotator::ZeroRotator, GridManager->GridToWorld(Candidate.Coord));
		AGridPickupActor* Pickup = GetWorld()->SpawnActorDeferred<AGridPickupActor>(
			SelectedSpawnEntry->PickupClass,
			SpawnTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Pickup)
		{
			UE_LOG(LogDungeonRunManager, Warning, TEXT("SpawnPickupsFromLayout failed at (%d,%d)."),
				Candidate.Coord.X, Candidate.Coord.Y);
			continue;
		}

		Pickup->FinishSpawning(SpawnTransform);
		Pickup->InitializeOnGrid(GridManager, Candidate.Coord);
		if (ResolvedPickupManager->RegisterPickup(Pickup))
		{
			++SpawnedCount;
		}
		else
		{
			Pickup->Destroy();
		}
	}
}

const FDungeonEnemySpawnEntry* ADungeonRunManager::SelectEnemySpawnEntryForCandidate(
	const FDungeonSpawnCandidate& Candidate,
	FRandomStream& Stream) const
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
			return &Entry;
		}
	}

	return nullptr;
}

const FDungeonPickupSpawnEntry* ADungeonRunManager::SelectPickupSpawnEntryForCandidate(
	const FDungeonSpawnCandidate& Candidate,
	FRandomStream& Stream) const
{
	if (!DungeonGenerationSettings)
	{
		return nullptr;
	}

	int32 TotalWeight = 0;
	for (const FDungeonPickupSpawnEntry& Entry : DungeonGenerationSettings->PickupSpawnPool)
	{
		if (!Entry.PickupClass || Entry.Weight <= 0)
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
	for (const FDungeonPickupSpawnEntry& Entry : DungeonGenerationSettings->PickupSpawnPool)
	{
		if (!Entry.PickupClass || Entry.Weight <= 0)
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
			return &Entry;
		}
	}

	return nullptr;
}

int32 ADungeonRunManager::CalculateEnemyKillThresholdForCandidate(
	const AGridEnemyPawn* Enemy,
	const FDungeonSpawnCandidate& Candidate,
	const FDungeonEnemySpawnEntry& SpawnEntry) const
{
	const int32 BaseThreshold = SpawnEntry.KillThresholdOverride > 0
		? SpawnEntry.KillThresholdOverride
		: (Enemy ? Enemy->KillThreshold : 1);
	const int32 DepthBonus = FMath::Max(0, Candidate.Depth) * FMath::Max(0, SpawnEntry.KillThresholdBonusPerDepth);
	return FMath::Max(1, BaseThreshold + DepthBonus);
}

void ADungeonRunManager::RegisterEnemyWithManager(AGridEnemyPawn* Enemy) const
{
	if (EnemyManager && Enemy)
	{
		EnemyManager->RegisterEnemy(Enemy);
	}
}

AGridPickupManager* ADungeonRunManager::EnsurePickupManager()
{
	if (PickupManager)
	{
		return PickupManager;
	}

	ResolveRuntimeReferences();
	if (PickupManager)
	{
		return PickupManager;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	PickupManager = World->SpawnActor<AGridPickupManager>(AGridPickupManager::StaticClass());
	return PickupManager;
}
