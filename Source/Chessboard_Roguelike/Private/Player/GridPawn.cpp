#include "Player/GridPawn.h"

#include "Combat/CombatResolverComponent.h"
#include "Combat/CombatTypes.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/TurnManager.h"
#include "Core/TurnStateTypes.h"
#include "Data/ChessPieceFormData.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Enemy/GridEnemyManager.h"
#include "Enemy/GridEnemyPawn.h"
#include "Grid/GridManager.h"
#include "Grid/TileEffectResolverComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Pickup/GridPickupManager.h"
#include "Player/ConversionEnergyComponent.h"
#include "Player/PlayerAttributeComponent.h"
#include "Player/PlayerTransformInventoryComponent.h"

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
	ConversionEnergyComponent = CreateDefaultSubobject<UConversionEnergyComponent>(TEXT("ConversionEnergyComponent"));
	TransformInventoryComponent = CreateDefaultSubobject<UPlayerTransformInventoryComponent>(TEXT("TransformInventoryComponent"));
}

void AGridPawn::BeginPlay()
{
	Super::BeginPlay();
	CacheDefaultPawnVisual();

	if (!bAutoInitializeOnBeginPlay || bInitializedOnGrid)
	{
		return;
	}

	AGridManager* FoundGridManager = nullptr;
	ATurnManager* FoundTurnManager = nullptr;
	AGridEnemyManager* FoundEnemyManager = nullptr;
	AGridPickupManager* FoundPickupManager = nullptr;

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

	for (TActorIterator<AGridPickupManager> It(GetWorld()); It; ++It)
	{
		FoundPickupManager = *It;
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

	if (FoundPickupManager)
	{
		PickupManager = FoundPickupManager;
	}

	if (bInitializedOnGrid)
	{
		FoundTurnManager->SetTurnState(ETurnState::PlayerInput);
		RefreshPlayerNextMoveTiles();
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
	SyncPlayerGridMaterialParameters();
	RefreshPlayerNextMoveTiles();
	bIsMoving = false;
	bInitializedOnGrid = true;
	SetActorTickEnabled(false);
}

void AGridPawn::TryMove(FIntPoint Direction)
{
	const int32 DirectionMagnitude = FMath::Abs(Direction.X) + FMath::Abs(Direction.Y);
	if (DirectionMagnitude != 1)
	{
		UE_LOG(LogGridPawn, Verbose, TEXT("TryMove ignored: direction (%d,%d) is not cardinal."), Direction.X, Direction.Y);
		return;
	}

	TryResolveMoveOrAttackToCoord(CurrentGridCoord + Direction);
}

bool AGridPawn::TryTransformMoveToCoord(FIntPoint TargetCoord, UChessPieceFormData* FormData)
{
	if (!IsLegalTransformTarget(TargetCoord, FormData))
	{
		return false;
	}

	const bool bAppliedVisualForThisMove = !bIsTransformVisualActive;
	if (bAppliedVisualForThisMove)
	{
		ApplyTransformVisual(FormData);
	}

	if (!TryResolveMoveOrAttackToCoord(TargetCoord))
	{
		if (bAppliedVisualForThisMove)
		{
			RestoreDefaultPawnVisual();
		}
		return false;
	}

	bRestoreDefaultVisualOnMoveFinish = bIsTransformVisualActive;
	return true;
}

TArray<FTransformMoveTarget> AGridPawn::BuildLegalTransformTargets(UChessPieceFormData* FormData) const
{
	TArray<FTransformMoveTarget> Targets;
	if (!GridManager || !FormData)
	{
		return Targets;
	}

	for (const FIntPoint& Direction : FormData->MoveDirections)
	{
		if (Direction == FIntPoint::ZeroValue)
		{
			continue;
		}

		if (FormData->bCanJump)
		{
			const FIntPoint CandidateCoord = CurrentGridCoord + Direction;

			FTileData TileData;
			if (!GridManager->GetTileData(CandidateCoord, TileData)
				|| !TileData.bWalkable
				|| TileData.TileType == ETileType::Obstacle)
			{
				continue;
			}

			if (TileData.OccupantType == EGridOccupantType::Empty)
			{
				FTransformMoveTarget Target;
				Target.Coord = CandidateCoord;
				Target.TargetType = ETransformTargetType::Move;
				Targets.Add(Target);
			}
			else if (TileData.OccupantType == EGridOccupantType::Enemy && FormData->bCanCaptureEnemy)
			{
				FTransformMoveTarget Target;
				Target.Coord = CandidateCoord;
				Target.TargetType = ETransformTargetType::Capture;
				Targets.Add(Target);
			}

			continue;
		}

		const int32 Range = FMath::Max(1, FormData->MaxRange);
		for (int32 Step = 1; Step <= Range; ++Step)
		{
			const FIntPoint CandidateCoord = CurrentGridCoord + FIntPoint(Direction.X * Step, Direction.Y * Step);

			FTileData TileData;
			if (!GridManager->GetTileData(CandidateCoord, TileData)
				|| !TileData.bWalkable
				|| TileData.TileType == ETileType::Obstacle)
			{
				break;
			}

			if (TileData.OccupantType == EGridOccupantType::Empty)
			{
				FTransformMoveTarget Target;
				Target.Coord = CandidateCoord;
				Target.TargetType = ETransformTargetType::Move;
				Targets.Add(Target);
				continue;
			}

			if (TileData.OccupantType == EGridOccupantType::Enemy && FormData->bCanCaptureEnemy)
			{
				FTransformMoveTarget Target;
				Target.Coord = CandidateCoord;
				Target.TargetType = ETransformTargetType::Capture;
				Targets.Add(Target);
			}

			break;
		}
	}

	return Targets;
}

bool AGridPawn::TryResolveMoveOrAttackToCoord(FIntPoint TargetCoord)
{
	if (!GridManager || !TurnManager)
	{
		UE_LOG(LogGridPawn, Warning, TEXT("TryResolveMoveOrAttackToCoord ignored: manager reference is null."));
		return false;
	}

	if (!TurnManager->CanAcceptPlayerInput() || bIsMoving)
	{
		// Both the turn state and local interpolation flag guard against repeated key presses.
		return false;
	}

	if (!GridManager->IsValidCoord(TargetCoord) || !GridManager->IsWalkable(TargetCoord))
	{
		// Illegal movement is intentionally silent and does not consume a step.
		return false;
	}

	FTileData TargetTileData;
	if (!GridManager->GetTileData(TargetCoord, TargetTileData))
	{
		return false;
	}

	if (TargetTileData.OccupantType == EGridOccupantType::Enemy)
	{
		AGridEnemyPawn* EnemyActor = Cast<AGridEnemyPawn>(TargetTileData.OccupantActor.Get());
		if (!EnemyActor)
		{
			UE_LOG(LogGridPawn, Warning, TEXT("TryMove found enemy occupancy at (%d,%d), but OccupantActor is not AGridEnemyPawn."),
				TargetCoord.X, TargetCoord.Y);
			return false;
		}

		return ResolveEnemyMeleeAttack(TargetCoord, EnemyActor);
	}

	if (TargetTileData.OccupantType != EGridOccupantType::Empty)
	{
		// Obstacles or unknown occupants are blocked without consuming a step.
		return false;
	}

	const FVector FromLocation = GetActorLocation();
	const FVector ToLocation = GridManager->GridToWorld(TargetCoord);

	TurnManager->BeginPlayerAction();
	GridManager->ClearPlayerNextMoveTiles();

	if (!GridManager->RequestMove(this, CurrentGridCoord, TargetCoord))
	{
		// GridManager is authoritative; restore input if the final atomic move check fails.
		TurnManager->EndPlayerAction();
		RefreshPlayerNextMoveTiles();
		return false;
	}

	CurrentGridCoord = TargetCoord;
	SyncPlayerGridMaterialParameters();
	if (TileEffectResolverComponent)
	{
		// Tile effects are resolved only after RequestMove succeeds, so failed movement never changes attributes.
		TileEffectResolverComponent->ResolveTileEnterEffect(this, CurrentGridCoord);
	}
	ResolvePickupAtCurrentTile();
	TurnManager->AddStep();
	StartVisualMove(FromLocation, ToLocation);
	return true;
}

bool AGridPawn::ResolveEnemyMeleeAttack(FIntPoint TargetCoord, AGridEnemyPawn* EnemyActor)
{
	if (!GridManager || !TurnManager || !EnemyActor)
	{
		return false;
	}

	const FVector FromLocation = GetActorLocation();
	const FVector TargetLocation = GridManager->GridToWorld(TargetCoord);

	TurnManager->BeginPlayerAction();
	GridManager->ClearPlayerNextMoveTiles();

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
			RefreshPlayerNextMoveTiles();
			return false;
		}

		CurrentGridCoord = TargetCoord;
		SyncPlayerGridMaterialParameters();
		if (TileEffectResolverComponent)
		{
			// A successful kill leaves the player occupying the target tile, so normal tile-enter effects apply.
			TileEffectResolverComponent->ResolveTileEnterEffect(this, CurrentGridCoord);
		}
		ResolvePickupAtCurrentTile();

		OnPlayerKilledEnemy(EnemyActor, DroppedEnergyType);

		TurnManager->AddStep();
		StartVisualMove(FromLocation, TargetLocation);
		return true;
	}

	// Failed attacks still spend the player's step but leave occupancy and tile effects unchanged.
	TurnManager->AddStep();
	StartFailedAttackVisualMove(FromLocation, TargetLocation);
	return true;
}

bool AGridPawn::IsLegalTransformTarget(FIntPoint TargetCoord, UChessPieceFormData* FormData) const
{
	if (!FormData)
	{
		return false;
	}

	const TArray<FTransformMoveTarget> LegalTargets = BuildLegalTransformTargets(FormData);
	return LegalTargets.ContainsByPredicate(
		[TargetCoord](const FTransformMoveTarget& Candidate)
		{
			return Candidate.Coord == TargetCoord;
		});
}

void AGridPawn::CacheDefaultPawnVisual()
{
	if (!PawnMesh)
	{
		return;
	}

	DefaultPawnMesh = PawnMesh->GetStaticMesh();
	DefaultPawnMaterial = PawnMesh->GetMaterial(0);
}

void AGridPawn::ApplyTransformVisual(UChessPieceFormData* FormData)
{
	if (!FormData)
	{
		return;
	}

	if (bIsTransformVisualActive && ActiveTransformVisualForm == FormData)
	{
		return;
	}

	if (bIsTransformVisualActive)
	{
		RestoreDefaultPawnVisual();
	}

	ActiveTransformVisualForm = FormData;
	bIsTransformVisualActive = true;

	if (PawnMesh)
	{
		if (!DefaultPawnMesh)
		{
			CacheDefaultPawnVisual();
		}

		if (FormData->FormMesh)
		{
			PawnMesh->SetStaticMesh(FormData->FormMesh);
		}

		if (FormData->FormMaterial)
		{
			PawnMesh->SetMaterial(0, FormData->FormMaterial);
		}
	}

	OnTransformVisualStateChanged.Broadcast(FormData, true);
	OnTransformVisualApplied(FormData);
}

void AGridPawn::RestoreDefaultPawnVisual()
{
	UChessPieceFormData* RestoredFormData = ActiveTransformVisualForm;

	if (PawnMesh)
	{
		if (DefaultPawnMesh)
		{
			PawnMesh->SetStaticMesh(DefaultPawnMesh);
		}

		if (DefaultPawnMaterial)
		{
			PawnMesh->SetMaterial(0, DefaultPawnMaterial);
		}
	}

	ActiveTransformVisualForm = nullptr;
	bIsTransformVisualActive = false;

	OnTransformVisualStateChanged.Broadcast(RestoredFormData, false);
	OnTransformVisualRestored(RestoredFormData);
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
	if (bRestoreDefaultVisualOnMoveFinish)
	{
		RestoreDefaultPawnVisual();
		bRestoreDefaultVisualOnMoveFinish = false;
	}
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

void AGridPawn::FindPickupManagerIfNeeded()
{
	if (PickupManager || !GetWorld())
	{
		return;
	}

	for (TActorIterator<AGridPickupManager> It(GetWorld()); It; ++It)
	{
		PickupManager = *It;
		break;
	}
}

void AGridPawn::ResolvePickupAtCurrentTile()
{
	FindPickupManagerIfNeeded();
	if (PickupManager)
	{
		PickupManager->TryCollectPickupAt(CurrentGridCoord, this);
	}
}

void AGridPawn::ResolvePostPlayerActionTurn()
{
	if (!TurnManager)
	{
		return;
	}

	if (TurnManager->CurrentTurnState == ETurnState::Victory || TurnManager->CurrentTurnState == ETurnState::Defeat)
	{
		return;
	}

	FindEnemyManagerIfNeeded();
	if (!EnemyManager)
	{
		TurnManager->EndPlayerAction();
		if (TurnManager->CanAcceptPlayerInput())
		{
			RefreshPlayerNextMoveTiles();
		}
		return;
	}

	TurnManager->BeginEnemyTurn();
	EnemyManager->ExecuteEnemyTurn();
	if (TurnManager->CurrentTurnState == ETurnState::Victory || TurnManager->CurrentTurnState == ETurnState::Defeat)
	{
		return;
	}
	if (!EnemyManager->ShouldDelayEnemyTurnEnd())
	{
		TurnManager->EndEnemyTurn();
		if (TurnManager->CanAcceptPlayerInput())
		{
			RefreshPlayerNextMoveTiles();
		}
	}
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

void AGridPawn::SyncPlayerGridMaterialParameters()
{
	if (!PlayerGridParameterCollection)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UMaterialParameterCollectionInstance* CollectionInstance =
		World->GetParameterCollectionInstance(PlayerGridParameterCollection);
	if (!CollectionInstance)
	{
		UE_LOG(LogGridPawn, Warning, TEXT("SyncPlayerGridMaterialParameters failed: collection instance is unavailable."));
		return;
	}

	CollectionInstance->SetScalarParameterValue(
		PlayerGridXParameterName,
		static_cast<float>(CurrentGridCoord.X));
	CollectionInstance->SetScalarParameterValue(
		PlayerGridYParameterName,
		static_cast<float>(CurrentGridCoord.Y));
}

void AGridPawn::RefreshPlayerNextMoveTiles()
{
	if (!GridManager)
	{
		return;
	}

	static const FIntPoint NeighborOffsets[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	TArray<FIntPoint> MoveCoords;
	MoveCoords.Reserve(UE_ARRAY_COUNT(NeighborOffsets));

	for (const FIntPoint& Offset : NeighborOffsets)
	{
		const FIntPoint CandidateCoord = CurrentGridCoord + Offset;

		FTileData TileData;
		if (!GridManager->GetTileData(CandidateCoord, TileData))
		{
			continue;
		}

		if (TileData.bWalkable
			&& TileData.TileType != ETileType::Obstacle
			&& TileData.OccupantType == EGridOccupantType::Empty)
		{
			MoveCoords.Add(CandidateCoord);
		}
	}

	GridManager->SetPlayerNextMoveTiles(MoveCoords);
}
