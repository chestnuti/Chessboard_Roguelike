#include "Player/GridPawn.h"

#include "Combat/CombatResolverComponent.h"
#include "Combat/CombatTypes.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/TurnManager.h"
#include "Core/TurnStateTypes.h"
#include "EngineUtils.h"
#include "Enemy/GridEnemyManager.h"
#include "Enemy/GridEnemyPawn.h"
#include "Grid/GridManager.h"
#include "Grid/TileEffectResolverComponent.h"
#include "Player/PlayerAttributeComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridPawn, Log, All);

namespace
{
	ETileType GetDroppedEnergyTypeForFaction(EEnemyFaction Faction)
	{
		switch (Faction)
		{
		case EEnemyFaction::Construct:
			return ETileType::Construct;
		case EEnemyFaction::Acid:
			return ETileType::Acid;
		default:
			return ETileType::Minimal;
		}
	}
}

AGridPawn::AGridPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PawnMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PawnMesh"));
	PawnMesh->SetupAttachment(SceneRoot);
	// Movement legality is grid-based, so the pawn mesh does not participate in blocking.
	PawnMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PlayerAttributeComponent = CreateDefaultSubobject<UPlayerAttributeComponent>(TEXT("PlayerAttributeComponent"));
	TileEffectResolverComponent = CreateDefaultSubobject<UTileEffectResolverComponent>(TEXT("TileEffectResolverComponent"));
	CombatResolverComponent = CreateDefaultSubobject<UCombatResolverComponent>(TEXT("CombatResolverComponent"));
}

void AGridPawn::BeginPlay()
{
	Super::BeginPlay();

	if (!bAutoInitializeOnBeginPlay || bInitializedOnGrid)
	{
		return;
	}

	AGridManager* FoundGridManager = nullptr;
	ATurnManager* FoundTurnManager = nullptr;
	AGridEnemyManager* FoundEnemyManager = nullptr;

	// Prototype convenience path: test maps can place managers without Blueprint wiring.
	for (TActorIterator<AGridManager> It(GetWorld()); It; ++It)
	{
		FoundGridManager = *It;
		break;
	}

	for (TActorIterator<ATurnManager> It(GetWorld()); It; ++It)
	{
		FoundTurnManager = *It;
		break;
	}

	for (TActorIterator<AGridEnemyManager> It(GetWorld()); It; ++It)
	{
		FoundEnemyManager = *It;
		break;
	}

	if (!FoundGridManager || !FoundTurnManager)
	{
		UE_LOG(LogGridPawn, Warning, TEXT("Auto initialization skipped: GridManager or TurnManager not found."));
		return;
	}

	if (FoundGridManager->Tiles.IsEmpty())
	{
		FoundGridManager->GenerateGrid();
	}

	InitializeOnGrid(FoundGridManager, FoundTurnManager, StartGridCoord);
	if (FoundEnemyManager)
	{
		EnemyManager = FoundEnemyManager;
		EnemyManager->InitializeEnemyManager(FoundGridManager, FoundTurnManager, this);
	}

	if (bInitializedOnGrid)
	{
		FoundTurnManager->SetTurnState(ETurnState::PlayerInput);
	}
}

void AGridPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsMoving)
	{
		return;
	}

	if (MoveDuration <= KINDA_SMALL_NUMBER)
	{
		// Zero duration still resolves cleanly and snaps to the authoritative grid center.
		FinishVisualMove();
		return;
	}

	// Tick is used only for presentation interpolation, not for reading input or deciding moves.
	MoveElapsedTime += DeltaSeconds;
	const float Alpha = FMath::Clamp(MoveElapsedTime / MoveDuration, 0.f, 1.f);
	if (bIsFailedAttackVisualMove)
	{
		const float ReboundAlpha = Alpha < 0.5f ? Alpha * 2.f : (Alpha - 0.5f) * 2.f;
		const FVector ReboundFrom = Alpha < 0.5f ? VisualMoveFrom : FailedAttackVisualPeak;
		const FVector ReboundTo = Alpha < 0.5f ? FailedAttackVisualPeak : VisualMoveTo;
		SetActorLocation(FMath::Lerp(ReboundFrom, ReboundTo, ReboundAlpha));
	}
	else
	{
		SetActorLocation(FMath::Lerp(VisualMoveFrom, VisualMoveTo, Alpha));
	}

	if (Alpha >= 1.f)
	{
		FinishVisualMove();
	}
}

void AGridPawn::InitializeOnGrid(AGridManager* InGridManager, ATurnManager* InTurnManager, FIntPoint InStartCoord)
{
	if (!InGridManager || !InTurnManager)
	{
		UE_LOG(LogGridPawn, Warning, TEXT("InitializeOnGrid failed: manager reference is null."));
		return;
	}

	if (InGridManager->Tiles.IsEmpty())
	{
		InGridManager->GenerateGrid();
	}

	if (!InGridManager->IsValidCoord(InStartCoord))
	{
		UE_LOG(LogGridPawn, Warning, TEXT("InitializeOnGrid failed: invalid start coord (%d,%d)."), InStartCoord.X, InStartCoord.Y);
		return;
	}

	if (bInitializedOnGrid && GridManager)
	{
		// Reinitialization moves the pawn to a new starting tile without leaving stale occupancy.
		GridManager->ClearOccupant(CurrentGridCoord);
	}

	GridManager = InGridManager;
	TurnManager = InTurnManager;
	if (TileEffectResolverComponent)
	{
		// Keep the resolver pointed at the same grid that owns movement validation and occupancy.
		TileEffectResolverComponent->SetGridManager(GridManager);
	}

	if (!GridManager->TryOccupyTile(InStartCoord, this, EGridOccupantType::Player))
	{
		UE_LOG(LogGridPawn, Warning, TEXT("InitializeOnGrid failed: could not occupy start coord (%d,%d)."),
			InStartCoord.X, InStartCoord.Y);
		return;
	}

	CurrentGridCoord = InStartCoord;
	SetActorLocation(GridManager->GridToWorld(CurrentGridCoord));
	bIsMoving = false;
	bInitializedOnGrid = true;
	SetActorTickEnabled(false);
}

void AGridPawn::TryMove(FIntPoint Direction)
{
	if (!GridManager || !TurnManager)
	{
		UE_LOG(LogGridPawn, Warning, TEXT("TryMove ignored: manager reference is null."));
		return;
	}

	if (!TurnManager->CanAcceptPlayerInput() || bIsMoving)
	{
		// Both the turn state and local interpolation flag guard against repeated key presses.
		return;
	}

	const int32 DirectionMagnitude = FMath::Abs(Direction.X) + FMath::Abs(Direction.Y);
	if (DirectionMagnitude != 1)
	{
		UE_LOG(LogGridPawn, Verbose, TEXT("TryMove ignored: direction (%d,%d) is not cardinal."), Direction.X, Direction.Y);
		return;
	}

	const FIntPoint TargetCoord = CurrentGridCoord + Direction;
	if (!GridManager->IsValidCoord(TargetCoord) || !GridManager->IsWalkable(TargetCoord))
	{
		// Illegal movement is intentionally silent and does not consume a step.
		return;
	}

	FTileData TargetTileData;
	if (!GridManager->GetTileData(TargetCoord, TargetTileData))
	{
		return;
	}

	if (TargetTileData.OccupantType == EGridOccupantType::Enemy)
	{
		AGridEnemyPawn* EnemyActor = Cast<AGridEnemyPawn>(TargetTileData.OccupantActor.Get());
		if (!EnemyActor)
		{
			UE_LOG(LogGridPawn, Warning, TEXT("TryMove found enemy occupancy at (%d,%d), but OccupantActor is not AGridEnemyPawn."),
				TargetCoord.X, TargetCoord.Y);
			return;
		}

		ResolveEnemyMeleeAttack(TargetCoord, EnemyActor);
		return;
	}

	if (TargetTileData.OccupantType != EGridOccupantType::Empty)
	{
		// Obstacles or unknown occupants are blocked without consuming a step.
		return;
	}

	const FVector FromLocation = GetActorLocation();
	const FVector ToLocation = GridManager->GridToWorld(TargetCoord);

	TurnManager->BeginPlayerAction();

	if (!GridManager->RequestMove(this, CurrentGridCoord, TargetCoord))
	{
		// GridManager is authoritative; restore input if the final atomic move check fails.
		TurnManager->EndPlayerAction();
		return;
	}

	CurrentGridCoord = TargetCoord;
	if (TileEffectResolverComponent)
	{
		// Tile effects are resolved only after RequestMove succeeds, so failed movement never changes attributes.
		TileEffectResolverComponent->ResolveTileEnterEffect(this, CurrentGridCoord);
	}
	TurnManager->AddStep();
	StartVisualMove(FromLocation, ToLocation);
}

void AGridPawn::ResolveEnemyMeleeAttack(FIntPoint TargetCoord, AGridEnemyPawn* EnemyActor)
{
	if (!GridManager || !TurnManager || !EnemyActor)
	{
		return;
	}

	const FVector FromLocation = GetActorLocation();
	const FVector TargetLocation = GridManager->GridToWorld(TargetCoord);

	TurnManager->BeginPlayerAction();

	const FCombatResolveResult CombatResult = CombatResolverComponent
		? CombatResolverComponent->ResolvePlayerMeleeAttack(this, EnemyActor)
		: FCombatResolveResult();

	if (CombatResult.bKilled)
	{
		const ETileType DroppedEnergyType = GetDroppedEnergyTypeForFaction(EnemyActor->Faction);

		EnemyActor->Kill();
		GridManager->ClearOccupant(TargetCoord);

		if (!GridManager->RequestMove(this, CurrentGridCoord, TargetCoord))
		{
			UE_LOG(LogGridPawn, Warning, TEXT("ResolveEnemyMeleeAttack killed enemy but failed to move player into (%d,%d)."),
				TargetCoord.X, TargetCoord.Y);
			TurnManager->EndPlayerAction();
			return;
		}

		CurrentGridCoord = TargetCoord;
		if (TileEffectResolverComponent)
		{
			// A successful kill leaves the player occupying the target tile, so normal tile-enter effects apply.
			TileEffectResolverComponent->ResolveTileEnterEffect(this, CurrentGridCoord);
		}

		OnPlayerKilledEnemy(EnemyActor, DroppedEnergyType);

		TurnManager->AddStep();
		StartVisualMove(FromLocation, TargetLocation);
		return;
	}

	// Failed attacks still spend the player's step but leave occupancy and tile effects unchanged.
	TurnManager->AddStep();
	StartFailedAttackVisualMove(FromLocation, TargetLocation);
}

void AGridPawn::StartVisualMove(const FVector& From, const FVector& To)
{
	VisualMoveFrom = From;
	VisualMoveTo = To;
	FailedAttackVisualPeak = To;
	MoveElapsedTime = 0.f;
	bIsFailedAttackVisualMove = false;
	bIsMoving = true;
	SetActorLocation(VisualMoveFrom);
	SetActorTickEnabled(true);
}

void AGridPawn::StartFailedAttackVisualMove(const FVector& From, const FVector& BlockedTarget)
{
	VisualMoveFrom = From;
	VisualMoveTo = From;
	FailedAttackVisualPeak = FMath::Lerp(From, BlockedTarget, 0.35f);
	MoveElapsedTime = 0.f;
	bIsFailedAttackVisualMove = true;
	bIsMoving = true;
	SetActorLocation(VisualMoveFrom);
	SetActorTickEnabled(true);
}

void AGridPawn::FinishVisualMove()
{
	// Snap at the end to prevent small interpolation drift from desyncing visuals and grid state.
	SetActorLocation(VisualMoveTo);
	bIsMoving = false;
	bIsFailedAttackVisualMove = false;
	MoveElapsedTime = 0.f;
	SetActorTickEnabled(false);

	ResolvePostPlayerActionTurn();
}

void AGridPawn::FindEnemyManagerIfNeeded()
{
	if (EnemyManager || !GetWorld())
	{
		return;
	}

	for (TActorIterator<AGridEnemyManager> It(GetWorld()); It; ++It)
	{
		EnemyManager = *It;
		if (EnemyManager)
		{
			EnemyManager->InitializeEnemyManager(GridManager, TurnManager, this);
		}
		break;
	}
}

void AGridPawn::ResolvePostPlayerActionTurn()
{
	if (!TurnManager)
	{
		return;
	}

	FindEnemyManagerIfNeeded();
	if (!EnemyManager)
	{
		TurnManager->EndPlayerAction();
		return;
	}

	TurnManager->BeginEnemyTurn();
	EnemyManager->ExecuteEnemyTurn();
	TurnManager->EndEnemyTurn();
}

bool AGridPawn::ConvertAreaAroundPlayer(AGridManager* InGridManager, ETileType EnergyType)
{
	if (!InGridManager)
	{
		return false;
	}
	const TArray<FIntPoint> AdjacentCoords = {
		CurrentGridCoord,
		CurrentGridCoord + FIntPoint(1, 0),
		CurrentGridCoord + FIntPoint(-1, 0),
		CurrentGridCoord + FIntPoint(0, 1),
		CurrentGridCoord + FIntPoint(0, -1),
		CurrentGridCoord + FIntPoint(1, 1),
		CurrentGridCoord + FIntPoint(1, -1),
		CurrentGridCoord + FIntPoint(-1, 1),
		CurrentGridCoord + FIntPoint(-1, -1)
	};
	bool bConvertedAny = false;
	for (const FIntPoint& Coord : AdjacentCoords)
	{
		if (InGridManager->IsTileConvertible(Coord))
		{
			InGridManager->SetTileType(Coord, EnergyType);
			bConvertedAny = true;
		}
	}
	return bConvertedAny;
}
