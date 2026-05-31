#include "Enemy/GridEnemyPawn.h"

#include "Combat/CombatResolverComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Grid/GridManager.h"
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
	SetActorLocation(FMath::Lerp(VisualMoveFrom, VisualMoveTo, Alpha));

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

bool AGridEnemyPawn::TryMoveToGridCoord(FIntPoint TargetCoord)
{
	if (!CanAct())
	{
		return false;
	}

	const FVector FromLocation = GetActorLocation();
	const FVector ToLocation = GridManager->GridToWorld(TargetCoord);

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

	const FIntPoint PlayerCoord = PlayerPawn->CurrentGridCoord;
	const FIntPoint Delta = PlayerCoord - CurrentGridCoord;
	const int32 ManhattanDistance = FMath::Abs(Delta.X) + FMath::Abs(Delta.Y);
	if (ManhattanDistance == 1)
	{
		bMeleeDamageResolvedInCurrentAttack = false;
		LastMeleeAttackResult = ApplyMeleeAttackDamage(PlayerPawn);
		ExecuteMeleeAttack(PlayerPawn);
		bMeleeDamageResolvedInCurrentAttack = false;
		return true;
	}

	TArray<FIntPoint> CandidateDirections;
	const FIntPoint HorizontalStep(FMath::Sign(Delta.X), 0);
	const FIntPoint VerticalStep(0, FMath::Sign(Delta.Y));

	if (FMath::Abs(Delta.X) >= FMath::Abs(Delta.Y))
	{
		if (Delta.X != 0)
		{
			CandidateDirections.Add(HorizontalStep);
		}
		if (Delta.Y != 0)
		{
			CandidateDirections.Add(VerticalStep);
		}
	}
	else
	{
		if (Delta.Y != 0)
		{
			CandidateDirections.Add(VerticalStep);
		}
		if (Delta.X != 0)
		{
			CandidateDirections.Add(HorizontalStep);
		}
	}

	for (const FIntPoint& Direction : CandidateDirections)
	{
		if (TryMoveToGridCoord(CurrentGridCoord + Direction))
		{
			return true;
		}
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
	if (!CanAct() || !PlayerPawn)
	{
		return FEnemyAttackResolveResult();
	}

	if (bMeleeDamageResolvedInCurrentAttack)
	{
		return LastMeleeAttackResult;
	}

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

	OnMeleeAttackResolved(PlayerPawn, AttackResult);

	return AttackResult;
}

void AGridEnemyPawn::Kill()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	bIsMoving = false;
	SetActorTickEnabled(false);
	UE_LOG(LogGridEnemyPawn, Log, TEXT("%s killed."), *GetNameSafe(this));
}

void AGridEnemyPawn::StartVisualMove(const FVector& From, const FVector& To)
{
	VisualMoveFrom = From;
	VisualMoveTo = To;
	MoveElapsedTime = 0.f;
	bIsMoving = true;
	SetActorLocation(VisualMoveFrom);
	SetActorTickEnabled(true);
}

void AGridEnemyPawn::FinishVisualMove()
{
	SetActorLocation(VisualMoveTo);
	bIsMoving = false;
	MoveElapsedTime = 0.f;
	SetActorTickEnabled(false);
}
