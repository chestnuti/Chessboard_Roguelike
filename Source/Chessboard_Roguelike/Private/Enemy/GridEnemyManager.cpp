#include "Enemy/GridEnemyManager.h"

#include "Core/TurnManager.h"
#include "Core/TurnStateTypes.h"
#include "EngineUtils.h"
#include "Enemy/GridEnemyPawn.h"
#include "Grid/GridManager.h"
#include "Player/GridPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridEnemyManager, Log, All);

AGridEnemyManager::AGridEnemyManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGridEnemyManager::BeginPlay()
{
	Super::BeginPlay();

	AutoInitializeReferences();
	RebuildEnemyList();
}

void AGridEnemyManager::InitializeEnemyManager(AGridManager* InGridManager, ATurnManager* InTurnManager, AGridPawn* InPlayerPawn)
{
	GridManager = InGridManager;
	TurnManager = InTurnManager;
	PlayerPawn = InPlayerPawn;

	if (!GridManager || !TurnManager || !PlayerPawn)
	{
		UE_LOG(LogGridEnemyManager, Warning, TEXT("InitializeEnemyManager received incomplete references."));
	}
}

void AGridEnemyManager::RegisterEnemy(AGridEnemyPawn* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	RegisteredEnemies.AddUnique(Enemy);
}

void AGridEnemyManager::UnregisterEnemy(AGridEnemyPawn* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	RegisteredEnemies.Remove(Enemy);
}

void AGridEnemyManager::RebuildEnemyList()
{
	RegisteredEnemies.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AGridEnemyPawn> It(World); It; ++It)
	{
		RegisterEnemy(*It);
	}

	PruneInvalidEnemies();
	UE_LOG(LogGridEnemyManager, Log, TEXT("Rebuilt enemy list: %d alive / %d registered."),
		GetAliveEnemies().Num(), RegisteredEnemies.Num());
}

void AGridEnemyManager::ExecuteEnemyTurn()
{
	AutoInitializeReferences();
	PruneInvalidEnemies();

	if (!GridManager || !TurnManager || !PlayerPawn)
	{
		UE_LOG(LogGridEnemyManager, Warning, TEXT("ExecuteEnemyTurn skipped: manager references are incomplete."));
		return;
	}

	if (TurnManager->CurrentTurnState != ETurnState::EnemyTurnResolve)
	{
		UE_LOG(LogGridEnemyManager, Verbose, TEXT("ExecuteEnemyTurn called while turn state is not EnemyTurnResolve."));
	}

	const TArray<TObjectPtr<AGridEnemyPawn>> EnemiesThisTurn = RegisteredEnemies;
	int32 ActedEnemyCount = 0;

	for (AGridEnemyPawn* Enemy : EnemiesThisTurn)
	{
		if (!IsValid(Enemy) || !Enemy->CanAct())
		{
			continue;
		}

		if (Enemy->ExecuteBasicTurn(PlayerPawn))
		{
			++ActedEnemyCount;
		}
	}

	PruneInvalidEnemies();
	UE_LOG(LogGridEnemyManager, Log, TEXT("Enemy turn resolved. Acted: %d, Alive: %d."),
		ActedEnemyCount, GetAliveEnemies().Num());
}

TArray<AGridEnemyPawn*> AGridEnemyManager::GetAliveEnemies() const
{
	TArray<AGridEnemyPawn*> AliveEnemies;
	for (AGridEnemyPawn* Enemy : RegisteredEnemies)
	{
		if (IsValid(Enemy) && Enemy->IsAlive())
		{
			AliveEnemies.Add(Enemy);
		}
	}

	return AliveEnemies;
}

void AGridEnemyManager::AutoInitializeReferences()
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

	if (GridManager && GridManager->Tiles.IsEmpty())
	{
		GridManager->GenerateGrid();
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
}

void AGridEnemyManager::PruneInvalidEnemies()
{
	RegisteredEnemies.RemoveAllSwap([](const TObjectPtr<AGridEnemyPawn>& Enemy)
	{
		return !IsValid(Enemy) || !Enemy->IsAlive();
	});
}
