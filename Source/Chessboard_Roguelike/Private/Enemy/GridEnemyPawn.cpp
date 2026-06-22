#include "Enemy/GridEnemyPawn.h"

#include "Audio/GameAudioSubsystem.h"
#include "Combat/CombatResolverComponent.h"
#include "Combat/CombatPreviewReceiver.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Grid/GridManager.h"
#include "Enemy/RangedAttackTelegraphComponent.h"
#include "Player/GridPawn.h"
#include "Player/PlayerAttributeComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridEnemyPawn, Log, All);

AGridEnemyPawn::AGridEnemyPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EnemyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EnemyMesh"));
	EnemyMesh->SetupAttachment(SceneRoot);
	// Grid occupancy, not mesh collision, owns gameplay blocking.
	EnemyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RangedAttackTelegraphComponent = CreateDefaultSubobject<URangedAttackTelegraphComponent>(TEXT("RangedAttackTelegraphComponent"));
}

void AGridEnemyPawn::BeginPlay()
{
	Super::BeginPlay();

	if (!bAutoInitializeOnBeginPlay || bInitializedOnGrid)
	{
		return;
	}

	AGridManager* FoundGridManager = nullptr;
	for (TActorIterator<AGridManager> It(GetWorld()); It; ++It)
	{
		FoundGridManager = *It;
		break;
	}

	if (!FoundGridManager)
	{
		UE_LOG(LogGridEnemyPawn, Warning, TEXT("Auto initialization skipped: GridManager not found."));
		return;
	}

	if (FoundGridManager->Tiles.IsEmpty())
	{
		FoundGridManager->GenerateGrid();
	}

	InitializeOnGrid(FoundGridManager, StartGridCoord);
}

void AGridEnemyPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsMoving)
	{
		return;
	}

	if (MoveDuration <= KINDA_SMALL_NUMBER)
	{
		FinishVisualMove();
		return;
	}

	MoveElapsedTime += DeltaSeconds;
	const float Alpha = FMath::Clamp(MoveElapsedTime / MoveDuration, 0.f, 1.f);
	if (bIsAttackReboundVisualMove)
	{
		const float ReboundAlpha = Alpha < 0.5f ? Alpha * 2.f : (Alpha - 0.5f) * 2.f;
		const FVector ReboundFrom = Alpha < 0.5f ? VisualMoveFrom : AttackReboundVisualPeak;
		const FVector ReboundTo = Alpha < 0.5f ? AttackReboundVisualPeak : VisualMoveTo;
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

void AGridEnemyPawn::InitializeOnGrid(AGridManager* InGridManager, FIntPoint InStartCoord)
{
	if (!InGridManager)
	{
		UE_LOG(LogGridEnemyPawn, Warning, TEXT("InitializeOnGrid failed: GridManager is null."));
		return;
	}

	if (InGridManager->Tiles.IsEmpty())
	{
		InGridManager->GenerateGrid();
	}

	if (!InGridManager->IsValidCoord(InStartCoord))
	{
		UE_LOG(LogGridEnemyPawn, Warning, TEXT("InitializeOnGrid failed: invalid start coord (%d,%d)."),
			InStartCoord.X, InStartCoord.Y);
		return;
	}

	if (bInitializedOnGrid && GridManager)
	{
		GridManager->ClearOccupant(CurrentGridCoord);
	}

	GridManager = InGridManager;
	if (!GridManager->TryOccupyTile(InStartCoord, this, EGridOccupantType::Enemy))
	{
		UE_LOG(LogGridEnemyPawn, Warning, TEXT("InitializeOnGrid failed: could not occupy start coord (%d,%d)."),
			InStartCoord.X, InStartCoord.Y);
		return;
	}

	CurrentGridCoord = InStartCoord;
	SetActorLocation(GridManager->GridToWorld(CurrentGridCoord));
	bIsMoving = false;
	bIsAttackReboundVisualMove = false;
	ClearRangedAimMode();
	SetActorTickEnabled(false);
	bDead = false;
	bInitializedOnGrid = true;
	SetActorHiddenInGame(false);
}

bool AGridEnemyPawn::CanReceiveDamage() const
{
	return !bDead;
}

bool AGridEnemyPawn::IsAlive() const
{
	return !bDead;
}

bool AGridEnemyPawn::CanAct() const
{
	return IsAlive() && GridManager != nullptr && !bIsMoving;
}

bool AGridEnemyPawn::IsSuppressedByPlayer(const AGridPawn* PlayerPawn) const
{
	if (!PlayerPawn)
	{
		return false;
	}

	const UPlayerAttributeComponent* AttributeComponent = PlayerPawn->PlayerAttributeComponent;
	if (!AttributeComponent)
	{
		AttributeComponent = PlayerPawn->FindComponentByClass<UPlayerAttributeComponent>();
	}

	return AttributeComponent && AttributeComponent->CanSuppressFaction(Faction);
}

bool AGridEnemyPawn::CanAttackCoordNextTurn(FIntPoint TargetCoord, const AGridPawn* PlayerPawn) const
{
	if (!IsAlive() || !GridManager || !PlayerPawn || IsSuppressedByPlayer(PlayerPawn))
	{
		return false;
	}

	if (BehaviorType == EEnemyBehaviorType::Ranged)
	{
		FIntPoint AttackDirection = FIntPoint::ZeroValue;
		TArray<FIntPoint> AttackTiles;
		return TryGetAxisDirection(CurrentGridCoord, TargetCoord, AttackDirection)
			&& BuildRangedLineFromCoord(CurrentGridCoord, AttackDirection, AttackTiles)
			&& AttackTiles.Contains(TargetCoord);
	}

	const FIntPoint Delta = TargetCoord - CurrentGridCoord;
	return FMath::Abs(Delta.X) + FMath::Abs(Delta.Y) == 1;
}

bool AGridEnemyPawn::TryMoveToGridCoord(FIntPoint TargetCoord)
{
	if (!CanAct())
	{
		return false;
	}

	const FVector FromLocation = GetActorLocation();
	const FVector ToLocation = GridManager->GridToWorld(TargetCoord);

	FTileData TargetTileData;
	if (GridManager->GetTileData(TargetCoord, TargetTileData)
		&& TargetTileData.OccupantType == EGridOccupantType::Enemy)
	{
		AGridEnemyPawn* TargetEnemy = Cast<AGridEnemyPawn>(TargetTileData.OccupantActor.Get());
		if (!TargetEnemy || TargetEnemy == this)
		{
			return false;
		}

		const FEnemyFriendlyFireResolveResult FriendlyFireResult =
			UCombatResolverComponent::ResolveEnemyMeleeCollision(this, TargetEnemy, TargetTileData.TileType);

		if (!FriendlyFireResult.bResolved
			|| FriendlyFireResult.bSameFaction
			|| (!FriendlyFireResult.bAttackerKilled && !FriendlyFireResult.bTargetKilled))
		{
			return false;
		}

		OnFriendlyFireResolved(TargetEnemy, FriendlyFireResult);
		TargetEnemy->OnFriendlyFireResolved(this, FriendlyFireResult);

		if (FriendlyFireResult.bTargetKilled)
		{
			GridManager->ClearOccupant(TargetCoord);
			TargetEnemy->Kill();
		}

		if (FriendlyFireResult.bAttackerKilled)
		{
			GridManager->ClearOccupant(CurrentGridCoord);
			Kill();
			return true;
		}

		if (FriendlyFireResult.bTargetKilled)
		{
			if (!GridManager->RequestMove(this, CurrentGridCoord, TargetCoord))
			{
				UE_LOG(LogGridEnemyPawn, Warning, TEXT("%s resolved friendly fire but failed to move into (%d,%d)."),
					*GetNameSafe(this), TargetCoord.X, TargetCoord.Y);
				return true;
			}

			CurrentGridCoord = TargetCoord;
			StartVisualMove(FromLocation, ToLocation);
			return true;
		}

		return true;
	}

	if (!GridManager->RequestMove(this, CurrentGridCoord, TargetCoord))
	{
		return false;
	}

	CurrentGridCoord = TargetCoord;
	StartVisualMove(FromLocation, ToLocation);
	return true;
}

bool AGridEnemyPawn::ExecuteBasicTurn_Implementation(AGridPawn* PlayerPawn)
{
	if (!CanAct() || !PlayerPawn)
	{
		return false;
	}

	if (IsSuppressedByPlayer(PlayerPawn))
	{
		OnSuppressedByPlayer(PlayerPawn);
		return false;
	}

	if (BehaviorType == EEnemyBehaviorType::Ranged)
	{
		if (HasPendingRangedAttack())
		{
			return ResolvePendingRangedAttack(PlayerPawn);
		}

		if (EnterRangedAimMode(PlayerPawn))
		{
			return true;
		}

		return TryMoveTowardRangedAlignment(PlayerPawn);
	}

	const FIntPoint PlayerCoord = PlayerPawn->CurrentGridCoord;
	const FIntPoint Delta = PlayerCoord - CurrentGridCoord;
	const int32 ManhattanDistance = FMath::Abs(Delta.X) + FMath::Abs(Delta.Y);
	if (ManhattanDistance == 1)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
			{
				AudioSubsystem->PlayEnemyMeleeAttackSFX(AudioProfile, GetActorLocation());
			}
		}

		bMeleeDamageResolvedInCurrentAttack = false;
		LastMeleeAttackResult = ApplyMeleeAttackDamage(PlayerPawn);
		ExecuteMeleeAttack(PlayerPawn);
		bMeleeDamageResolvedInCurrentAttack = false;
		return true;
	}

	TArray<FIntPoint> PathToPlayer;
	if (GridManager->FindPathAStar(CurrentGridCoord, PlayerCoord, PathToPlayer, true) && PathToPlayer.Num() >= 2)
	{
		return TryMoveToGridCoord(PathToPlayer[1]);
	}

	return false;
}

bool AGridEnemyPawn::HasPendingRangedAttack() const
{
	return ActionState == EEnemyActionState::AimingRangedAttack && !PendingRangedAttackTiles.IsEmpty();
}

bool AGridEnemyPawn::ResolvePendingRangedAttack(AGridPawn* PlayerPawn)
{
	if (!IsAlive() || !PlayerPawn || !HasPendingRangedAttack())
	{
		ClearRangedAimMode();
		return false;
	}

	if (IsSuppressedByPlayer(PlayerPawn))
	{
		OnSuppressedByPlayer(PlayerPawn);
		ClearRangedAimMode();
		return false;
	}

	const TArray<FIntPoint> AttackTiles = PendingRangedAttackTiles;
	const bool bHitPlayer = AttackTiles.Contains(PlayerPawn->CurrentGridCoord);
	FEnemyAttackResolveResult AttackResult;

	PlayRangedAttackAudio();

	if (bHitPlayer)
	{
		AttackResult = ApplyRangedAttackDamage(PlayerPawn);
	}

	for (const FIntPoint& TileCoord : AttackTiles)
	{
		if (!GridManager || TileCoord == CurrentGridCoord || TileCoord == PlayerPawn->CurrentGridCoord)
		{
			continue;
		}

		FTileData TileData;
		if (!GridManager->GetTileData(TileCoord, TileData) || TileData.OccupantType != EGridOccupantType::Enemy)
		{
			continue;
		}

		AGridEnemyPawn* TargetEnemy = Cast<AGridEnemyPawn>(TileData.OccupantActor.Get());
		if (TargetEnemy && TargetEnemy != this)
		{
			ApplyRangedFriendlyFireDamage(TargetEnemy);
		}
	}

	ClearRangedAimMode();
	OnRangedAttackResolved(PlayerPawn, AttackTiles, AttackResult, bHitPlayer);
	return true;
}

void AGridEnemyPawn::PlayRangedAttackAudio()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
		{
			AudioSubsystem->PlayEnemyRangedAttackSFX(AudioProfile, GetActorLocation());
		}
	}
}

void AGridEnemyPawn::ClearRangedAimMode()
{
	const bool bWasAiming = ActionState == EEnemyActionState::AimingRangedAttack || !PendingRangedAttackTiles.IsEmpty();
	ActionState = EEnemyActionState::Idle;
	PendingRangedAttackTiles.Reset();
	PendingRangedAttackDirection = FIntPoint::ZeroValue;

	if (RangedAttackTelegraphComponent)
	{
		RangedAttackTelegraphComponent->ClearTelegraph();
	}

	if (bWasAiming)
	{
		OnRangedAimCleared();
	}
}

bool AGridEnemyPawn::EnterRangedAimMode(AGridPawn* PlayerPawn)
{
	TArray<FIntPoint> AttackTiles;
	FIntPoint AttackDirection = FIntPoint::ZeroValue;
	if (!HasLineOfSightToPlayer(PlayerPawn, AttackTiles, AttackDirection))
	{
		return false;
	}

	ActionState = EEnemyActionState::AimingRangedAttack;
	PendingRangedAttackTiles = AttackTiles;
	PendingRangedAttackDirection = AttackDirection;

	if (RangedAttackTelegraphComponent)
	{
		RangedAttackTelegraphComponent->ShowTelegraph(GridManager, PendingRangedAttackTiles);
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
		{
			AudioSubsystem->PlayEnemyRangedAimSFX(AudioProfile, GetActorLocation());
		}
	}

	OnRangedAimStarted(PlayerPawn, PendingRangedAttackTiles);
	return true;
}

bool AGridEnemyPawn::TryMoveTowardRangedAlignment(AGridPawn* PlayerPawn)
{
	if (!CanAct() || !PlayerPawn || !GridManager)
	{
		return false;
	}

	static const FIntPoint NeighborOffsets[] =
	{
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1)
	};

	const FIntPoint PlayerCoord = PlayerPawn->CurrentGridCoord;
	FIntPoint CurrentDirection = FIntPoint::ZeroValue;
	if (TryGetAxisDirection(CurrentGridCoord, PlayerCoord, CurrentDirection))
	{
		TArray<FIntPoint> CurrentLine;
		const bool bCanAttackFromCurrentCoord = BuildRangedLineFromCoord(CurrentGridCoord, CurrentDirection, CurrentLine)
			&& CurrentLine.Contains(PlayerCoord);
		if (!bCanAttackFromCurrentCoord)
		{
			return TryMoveTowardPlayerByAStar(PlayerPawn);
		}
	}

	FIntPoint BestTarget = CurrentGridCoord;
	int32 BestScore = MAX_int32;

	for (const FIntPoint& Offset : NeighborOffsets)
	{
		const FIntPoint Candidate = CurrentGridCoord + Offset;
		FTileData CandidateTile;
		if (!GridManager->GetTileData(Candidate, CandidateTile)
			|| !CandidateTile.bWalkable
			|| CandidateTile.TileType == ETileType::Obstacle
			|| CandidateTile.OccupantType != EGridOccupantType::Empty)
		{
			continue;
		}

		FIntPoint CandidateDirection = FIntPoint::ZeroValue;
		TArray<FIntPoint> CandidateLine;
		const bool bSameAxis = TryGetAxisDirection(Candidate, PlayerCoord, CandidateDirection);
		const bool bHasLineOfSight = bSameAxis
			&& BuildRangedLineFromCoord(Candidate, CandidateDirection, CandidateLine)
			&& CandidateLine.Contains(PlayerCoord);

		if (!bHasLineOfSight)
		{
			continue;
		}

		const FIntPoint Delta = PlayerCoord - Candidate;
		const int32 ManhattanDistance = FMath::Abs(Delta.X) + FMath::Abs(Delta.Y);
		const int32 CandidateScore = ManhattanDistance;

		if (CandidateScore < BestScore)
		{
			BestScore = CandidateScore;
			BestTarget = Candidate;
		}
	}

	if (BestTarget != CurrentGridCoord)
	{
		return TryMoveToGridCoord(BestTarget);
	}

	return TryMoveTowardPlayerByAStar(PlayerPawn);
}

bool AGridEnemyPawn::TryMoveTowardPlayerByAStar(AGridPawn* PlayerPawn)
{
	if (!CanAct() || !PlayerPawn || !GridManager)
	{
		return false;
	}

	TArray<FIntPoint> PathToPlayer;
	if (GridManager->FindPathAStar(CurrentGridCoord, PlayerPawn->CurrentGridCoord, PathToPlayer, true) && PathToPlayer.Num() >= 2)
	{
		return TryMoveToGridCoord(PathToPlayer[1]);
	}

	return false;
}

bool AGridEnemyPawn::HasLineOfSightToPlayer(
	const AGridPawn* PlayerPawn,
	TArray<FIntPoint>& OutLineTiles,
	FIntPoint& OutDirection) const
{
	OutLineTiles.Reset();
	OutDirection = FIntPoint::ZeroValue;

	if (!GridManager || !PlayerPawn)
	{
		return false;
	}

	if (!TryGetAxisDirection(CurrentGridCoord, PlayerPawn->CurrentGridCoord, OutDirection))
	{
		return false;
	}

	if (!BuildRangedLineFromCoord(CurrentGridCoord, OutDirection, OutLineTiles))
	{
		return false;
	}

	return OutLineTiles.Contains(PlayerPawn->CurrentGridCoord);
}

bool AGridEnemyPawn::BuildRangedLineFromCoord(
	FIntPoint StartCoord,
	FIntPoint Direction,
	TArray<FIntPoint>& OutLineTiles) const
{
	OutLineTiles.Reset();

	if (!GridManager || Direction == FIntPoint::ZeroValue)
	{
		return false;
	}

	const int32 MaxSteps = MaxRangedAttackDistance > 0 ? MaxRangedAttackDistance : MAX_int32;
	FIntPoint Cursor = StartCoord + Direction;
	for (int32 Step = 0; Step < MaxSteps && GridManager->IsValidCoord(Cursor); ++Step)
	{
		FTileData TileData;
		if (!GridManager->GetTileData(Cursor, TileData)
			|| TileData.TileType == ETileType::Obstacle
			|| TileData.OccupantType == EGridOccupantType::Obstacle)
		{
			break;
		}

		OutLineTiles.Add(Cursor);
		Cursor = Cursor + Direction;
	}

	return !OutLineTiles.IsEmpty();
}

bool AGridEnemyPawn::TryGetAxisDirection(FIntPoint FromCoord, FIntPoint ToCoord, FIntPoint& OutDirection)
{
	OutDirection = FIntPoint::ZeroValue;

	if (FromCoord.X == ToCoord.X && FromCoord.Y != ToCoord.Y)
	{
		OutDirection = FIntPoint(0, ToCoord.Y > FromCoord.Y ? 1 : -1);
		return true;
	}

	if (FromCoord.Y == ToCoord.Y && FromCoord.X != ToCoord.X)
	{
		OutDirection = FIntPoint(ToCoord.X > FromCoord.X ? 1 : -1, 0);
		return true;
	}

	return false;
}

void AGridEnemyPawn::ExecuteMeleeAttack_Implementation(AGridPawn* PlayerPawn)
{
	const FEnemyAttackResolveResult AttackResult = ApplyMeleeAttackDamage(PlayerPawn);

	UE_LOG(LogGridEnemyPawn, Log, TEXT("%s attacks %s for %d HP damage. Remaining HP: %d."),
		*GetNameSafe(this),
		*GetNameSafe(PlayerPawn),
		AttackResult.AppliedHealthDamage,
		AttackResult.RemainingHealth);
}

FEnemyAttackResolveResult AGridEnemyPawn::ApplyMeleeAttackDamage(AGridPawn* PlayerPawn)
{
	if (!PlayerPawn)
	{
		return FEnemyAttackResolveResult();
	}

	if (bMeleeDamageResolvedInCurrentAttack)
	{
		return LastMeleeAttackResult;
	}

	if (!CanAct())
	{
		return FEnemyAttackResolveResult();
	}

	const FVector FromLocation = GetActorLocation();
	const FVector PlayerLocation = PlayerPawn->GetActorLocation();

	UCombatResolverComponent* CombatResolver = PlayerPawn->CombatResolverComponent;
	if (!CombatResolver)
	{
		CombatResolver = PlayerPawn->FindComponentByClass<UCombatResolverComponent>();
	}

	FEnemyAttackResolveResult AttackResult;
	if (CombatResolver)
	{
		AttackResult = CombatResolver->ResolveEnemyMeleeAttack(this, PlayerPawn);
	}
	else
	{
		UE_LOG(LogGridEnemyPawn, Warning, TEXT("%s could not damage %s: CombatResolverComponent not found."),
			*GetNameSafe(this), *GetNameSafe(PlayerPawn));
	}

	LastMeleeAttackResult = AttackResult;
	bMeleeDamageResolvedInCurrentAttack = true;

	if (AttackResult.bPlayerDefeated)
	{
		ResolveDefeatedPlayerOccupancy(PlayerPawn);
	}
	else if (AttackResult.bDamageApplied)
	{
		StartAttackReboundVisualMove(FromLocation, PlayerLocation);
	}

	OnMeleeAttackResolved(PlayerPawn, AttackResult);

	return AttackResult;
}

FEnemyAttackResolveResult AGridEnemyPawn::ApplyRangedAttackDamage(AGridPawn* PlayerPawn)
{
	if (!PlayerPawn || !IsAlive())
	{
		return FEnemyAttackResolveResult();
	}

	UCombatResolverComponent* CombatResolver = PlayerPawn->CombatResolverComponent;
	if (!CombatResolver)
	{
		CombatResolver = PlayerPawn->FindComponentByClass<UCombatResolverComponent>();
	}

	if (!CombatResolver)
	{
		UE_LOG(LogGridEnemyPawn, Warning, TEXT("%s could not ranged-damage %s: CombatResolverComponent not found."),
			*GetNameSafe(this), *GetNameSafe(PlayerPawn));
		return FEnemyAttackResolveResult();
	}

	return CombatResolver->ResolveEnemyMeleeAttack(this, PlayerPawn);
}

bool AGridEnemyPawn::ApplyRangedFriendlyFireDamage(AGridEnemyPawn* TargetEnemy)
{
	if (!IsAlive() || !TargetEnemy || TargetEnemy == this)
	{
		return false;
	}

	const FEnemyFriendlyFireResolveResult FriendlyFireResult =
		UCombatResolverComponent::ResolveEnemyRangedFriendlyFire(this, TargetEnemy);

	if (!FriendlyFireResult.bResolved
		|| FriendlyFireResult.bSameFaction
		|| !FriendlyFireResult.bTargetKilled)
	{
		return false;
	}

	OnFriendlyFireResolved(TargetEnemy, FriendlyFireResult);
	TargetEnemy->OnFriendlyFireResolved(this, FriendlyFireResult);

	AGridManager* TargetGridManager = TargetEnemy->GridManager ? TargetEnemy->GridManager.Get() : GridManager.Get();
	if (TargetGridManager)
	{
		TargetGridManager->ClearOccupant(TargetEnemy->CurrentGridCoord);
	}

	TargetEnemy->Kill();
	return true;
}

void AGridEnemyPawn::Kill()
{
	if (bDead)
	{
		return;
	}

	const FIntPoint DeathCoord = CurrentGridCoord;
	const FVector DeathWorldLocation = GridManager ? GridManager->GridToWorld(DeathCoord) : GetActorLocation();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
		{
			if (BehaviorType == EEnemyBehaviorType::Ranged)
			{
				AudioSubsystem->PlayEnemyRangedDeathSFX(AudioProfile, DeathWorldLocation);
			}
			else
			{
				AudioSubsystem->PlayEnemyMeleeDeathSFX(AudioProfile, DeathWorldLocation);
			}
		}
	}

	bDead = true;
	if (GetClass()->ImplementsInterface(UCombatPreviewReceiver::StaticClass()))
	{
		ICombatPreviewReceiver::Execute_UpdateCombatPreview(this, FCombatPreviewState());
	}

	OnGridEnemyKilled.Broadcast(this, DeathCoord, DeathWorldLocation);

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	ClearRangedAimMode();
	bIsMoving = false;
	bIsAttackReboundVisualMove = false;
	SetActorTickEnabled(false);
	UE_LOG(LogGridEnemyPawn, Log, TEXT("%s killed."), *GetNameSafe(this));
}

void AGridEnemyPawn::StartVisualMove(const FVector& From, const FVector& To)
{
	VisualMoveFrom = From;
	VisualMoveTo = To;
	AttackReboundVisualPeak = To;
	MoveElapsedTime = 0.f;
	bIsAttackReboundVisualMove = false;
	bIsMoving = true;
	SetActorLocation(VisualMoveFrom);
	SetActorTickEnabled(true);
}

void AGridEnemyPawn::FinishVisualMove()
{
	SetActorLocation(VisualMoveTo);
	bIsMoving = false;
	bIsAttackReboundVisualMove = false;
	MoveElapsedTime = 0.f;
	SetActorTickEnabled(false);
}

void AGridEnemyPawn::StartAttackReboundVisualMove(const FVector& From, const FVector& BlockedTarget)
{
	VisualMoveFrom = From;
	VisualMoveTo = From;
	AttackReboundVisualPeak = FMath::Lerp(From, BlockedTarget, 0.35f);
	MoveElapsedTime = 0.f;
	bIsAttackReboundVisualMove = true;
	bIsMoving = true;
	SetActorLocation(VisualMoveFrom);
	SetActorTickEnabled(true);
}

void AGridEnemyPawn::ResolveDefeatedPlayerOccupancy(AGridPawn* PlayerPawn)
{
	if (!GridManager || !PlayerPawn)
	{
		return;
	}

	const FIntPoint TargetCoord = PlayerPawn->CurrentGridCoord;
	if (!GridManager->IsValidCoord(TargetCoord) || !GridManager->IsWalkable(TargetCoord))
	{
		UE_LOG(LogGridEnemyPawn, Warning, TEXT("%s cannot occupy defeated player coord (%d,%d): invalid or blocked target."),
			*GetNameSafe(this), TargetCoord.X, TargetCoord.Y);
		return;
	}

	const FVector FromLocation = GetActorLocation();
	const FVector TargetLocation = GridManager->GridToWorld(TargetCoord);

	GridManager->ClearOccupant(TargetCoord);
	if (!GridManager->RequestMove(this, CurrentGridCoord, TargetCoord))
	{
		UE_LOG(LogGridEnemyPawn, Warning, TEXT("%s defeated player but failed to move into (%d,%d)."),
			*GetNameSafe(this), TargetCoord.X, TargetCoord.Y);
		StartAttackReboundVisualMove(FromLocation, TargetLocation);
		return;
	}

	CurrentGridCoord = TargetCoord;
	StartVisualMove(FromLocation, TargetLocation);
}
