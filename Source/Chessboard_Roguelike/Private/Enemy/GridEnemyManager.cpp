#include "Enemy/GridEnemyManager.h"

#include "Core/TurnManager.h"
#include "Core/TurnStateTypes.h"
#include "Combat/CombatResolverComponent.h"
#include "EngineUtils.h"
#include "Enemy/GridEnemyPawn.h"
#include "Grid/GridManager.h"
#include "Player/ConversionEnergyComponent.h"
#include "Player/GridPlayerController.h"
#include "Player/GridPawn.h"
#include "Player/PlayerAttributeComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridEnemyManager, Log, All);

namespace
{
	struct FQueuedRangedFriendlyFire
	{
		AGridEnemyPawn* Attacker = nullptr;
		AGridEnemyPawn* Target = nullptr;
		FEnemyFriendlyFireResolveResult ResolveResult;
	};

	struct FQueuedRangedAttack
	{
		AGridEnemyPawn* Attacker = nullptr;
		TArray<FIntPoint> AttackTiles;
		bool bHitPlayer = false;
		FEnemyAttackResolveResult PlayerAttackResult;
		TArray<FQueuedRangedFriendlyFire> FriendlyFireHits;
	};
}

AGridEnemyManager::AGridEnemyManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AGridEnemyManager::BeginPlay()
{
	Super::BeginPlay();

	AutoInitializeReferences();
	RebuildEnemyList();
}

void AGridEnemyManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bWaitingForEnemyMovement)
	{
		SetActorTickEnabled(false);
		return;
	}

	if (TurnManager && TurnManager->CurrentTurnState == ETurnState::Defeat)
	{
		bWaitingForEnemyMovement = false;
		SetActorTickEnabled(false);
		return;
	}

	if (HasMovingEnemies())
	{
		return;
	}

	bWaitingForEnemyMovement = false;
	SetActorTickEnabled(false);

	if (TurnManager && TurnManager->CurrentTurnState == ETurnState::EnemyTurnResolve)
	{
		TurnManager->EndEnemyTurn();
		if (PlayerPawn && TurnManager->CanAcceptPlayerInput())
		{
			PlayerPawn->RefreshPlayerNextMoveTiles();
		}
	}
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
	Enemy->OnGridEnemyKilled.RemoveDynamic(this, &AGridEnemyManager::HandleEnemyKilled);
	Enemy->OnGridEnemyKilled.AddDynamic(this, &AGridEnemyManager::HandleEnemyKilled);
}

void AGridEnemyManager::UnregisterEnemy(AGridEnemyPawn* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	Enemy->OnGridEnemyKilled.RemoveDynamic(this, &AGridEnemyManager::HandleEnemyKilled);
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

	const UPlayerAttributeComponent* PlayerAttributes = PlayerPawn->PlayerAttributeComponent;
	if (!PlayerAttributes)
	{
		PlayerAttributes = PlayerPawn->FindComponentByClass<UPlayerAttributeComponent>();
	}

	if (PlayerAttributes && PlayerAttributes->IsDefeated())
	{
		TurnManager->SetTurnState(ETurnState::Defeat);
		bWaitingForEnemyMovement = false;
		SetActorTickEnabled(false);
		return;
	}

	const TArray<TObjectPtr<AGridEnemyPawn>> EnemiesThisTurn = RegisteredEnemies;
	TSet<AGridEnemyPawn*> ResolvedRangedAttackers;
	int32 ActedEnemyCount = ResolvePendingRangedAttacks(ResolvedRangedAttackers);

	if (PlayerAttributes && PlayerAttributes->IsDefeated())
	{
		TurnManager->SetTurnState(ETurnState::Defeat);
	}

	if (TurnManager->CurrentTurnState != ETurnState::Defeat)
	{
		for (AGridEnemyPawn* Enemy : EnemiesThisTurn)
		{
			if (!IsValid(Enemy) || !Enemy->CanAct() || Enemy->HasPendingRangedAttack() || ResolvedRangedAttackers.Contains(Enemy))
			{
				continue;
			}

			if (Enemy->ExecuteBasicTurn(PlayerPawn))
			{
				++ActedEnemyCount;
			}

			if (PlayerAttributes && PlayerAttributes->IsDefeated())
			{
				TurnManager->SetTurnState(ETurnState::Defeat);
				break;
			}
		}
	}

	if (TurnManager->CurrentTurnState != ETurnState::Defeat && HasMovingEnemies())
	{
		bWaitingForEnemyMovement = true;
		SetActorTickEnabled(true);
	}
	else
	{
		bWaitingForEnemyMovement = false;
		SetActorTickEnabled(false);
	}

	PruneInvalidEnemies();
	UE_LOG(LogGridEnemyManager, Log, TEXT("Enemy turn resolved. Acted: %d, Alive: %d."),
		ActedEnemyCount, GetAliveEnemies().Num());
}

bool AGridEnemyManager::ShouldDelayEnemyTurnEnd() const
{
	return bWaitingForEnemyMovement;
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

void AGridEnemyManager::HandleEnemyKilled(AGridEnemyPawn* Enemy, FIntPoint DeathCoord, FVector DeathWorldLocation)
{
	AutoInitializeReferences();

	if (PlayerPawn && PlayerPawn->ConversionEnergyComponent)
	{
		PlayerPawn->ConversionEnergyComponent->GrantConversionEnergy();
	}

	AGridPlayerController* GridPlayerController = PlayerPawn
		? Cast<AGridPlayerController>(PlayerPawn->GetController())
		: nullptr;
	if (!GridPlayerController)
	{
		UE_LOG(LogGridEnemyManager, Verbose, TEXT("Enemy death camera focus skipped: GridPlayerController not found."));
		return;
	}

	GridPlayerController->FocusCombatCameraOnGridTile(DeathWorldLocation);
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

bool AGridEnemyManager::HasMovingEnemies() const
{
	for (const AGridEnemyPawn* Enemy : RegisteredEnemies)
	{
		if (IsValid(Enemy) && Enemy->bIsMoving)
		{
			return true;
		}
	}

	return false;
}

int32 AGridEnemyManager::ResolvePendingRangedAttacks(TSet<AGridEnemyPawn*>& OutResolvedAttackers)
{
	if (!PlayerPawn)
	{
		return 0;
	}

	TArray<FQueuedRangedAttack> QueuedAttacks;
	TArray<FQueuedRangedFriendlyFire> QueuedFriendlyFireHits;
	TSet<AGridEnemyPawn*> EnemiesToKill;
	const TArray<TObjectPtr<AGridEnemyPawn>> EnemiesThisTurn = RegisteredEnemies;
	for (AGridEnemyPawn* Enemy : EnemiesThisTurn)
	{
		if (!IsValid(Enemy) || !Enemy->CanAct() || !Enemy->HasPendingRangedAttack())
		{
			continue;
		}

		OutResolvedAttackers.Add(Enemy);

		if (Enemy->IsSuppressedByPlayer(PlayerPawn))
		{
			Enemy->OnSuppressedByPlayer(PlayerPawn);
			Enemy->ClearRangedAimMode();
			continue;
		}

		FQueuedRangedAttack& QueuedAttack = QueuedAttacks.AddDefaulted_GetRef();
		QueuedAttack.Attacker = Enemy;
		QueuedAttack.AttackTiles = Enemy->PendingRangedAttackTiles;
		QueuedAttack.bHitPlayer = QueuedAttack.AttackTiles.Contains(PlayerPawn->CurrentGridCoord);

		for (const FIntPoint& TileCoord : QueuedAttack.AttackTiles)
		{
			if (!GridManager || TileCoord == Enemy->CurrentGridCoord || TileCoord == PlayerPawn->CurrentGridCoord)
			{
				continue;
			}

			FTileData TileData;
			if (!GridManager->GetTileData(TileCoord, TileData) || TileData.OccupantType != EGridOccupantType::Enemy)
			{
				continue;
			}

			AGridEnemyPawn* TargetEnemy = Cast<AGridEnemyPawn>(TileData.OccupantActor.Get());
			if (!IsValid(TargetEnemy) || TargetEnemy == Enemy)
			{
				continue;
			}

			const FEnemyFriendlyFireResolveResult FriendlyFireResult =
				UCombatResolverComponent::ResolveEnemyRangedFriendlyFire(Enemy, TargetEnemy);
			if (!FriendlyFireResult.bResolved
				|| FriendlyFireResult.bSameFaction
				|| !FriendlyFireResult.bTargetKilled)
			{
				continue;
			}

			FQueuedRangedFriendlyFire& FriendlyFireHit = QueuedAttack.FriendlyFireHits.AddDefaulted_GetRef();
			FriendlyFireHit.Attacker = Enemy;
			FriendlyFireHit.Target = TargetEnemy;
			FriendlyFireHit.ResolveResult = FriendlyFireResult;
			QueuedFriendlyFireHits.Add(FriendlyFireHit);
			EnemiesToKill.Add(TargetEnemy);
		}
	}

	for (FQueuedRangedAttack& QueuedAttack : QueuedAttacks)
	{
		if (!IsValid(QueuedAttack.Attacker))
		{
			continue;
		}

		if (QueuedAttack.bHitPlayer)
		{
			QueuedAttack.PlayerAttackResult = QueuedAttack.Attacker->ApplyRangedAttackDamage(PlayerPawn);
		}
	}

	for (const FQueuedRangedFriendlyFire& FriendlyFireHit : QueuedFriendlyFireHits)
	{
		if (!IsValid(FriendlyFireHit.Attacker) || !IsValid(FriendlyFireHit.Target))
		{
			continue;
		}

		FriendlyFireHit.Attacker->OnFriendlyFireResolved(FriendlyFireHit.Target, FriendlyFireHit.ResolveResult);
		FriendlyFireHit.Target->OnFriendlyFireResolved(FriendlyFireHit.Attacker, FriendlyFireHit.ResolveResult);
	}

	for (const FQueuedRangedAttack& QueuedAttack : QueuedAttacks)
	{
		if (!IsValid(QueuedAttack.Attacker))
		{
			continue;
		}

		QueuedAttack.Attacker->ClearRangedAimMode();
		QueuedAttack.Attacker->OnRangedAttackResolved(
			PlayerPawn,
			QueuedAttack.AttackTiles,
			QueuedAttack.PlayerAttackResult,
			QueuedAttack.bHitPlayer);
	}

	for (AGridEnemyPawn* EnemyToKill : EnemiesToKill)
	{
		if (!IsValid(EnemyToKill) || !EnemyToKill->IsAlive())
		{
			continue;
		}

		AGridManager* EnemyGridManager = EnemyToKill->GridManager ? EnemyToKill->GridManager.Get() : GridManager.Get();
		if (EnemyGridManager)
		{
			EnemyGridManager->ClearOccupant(EnemyToKill->CurrentGridCoord);
		}

		EnemyToKill->Kill();
	}

	return QueuedAttacks.Num();
}
