#include "Enemy/GridEnemyPawn.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Grid/GridManager.h"
#include "Player/GridPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridEnemyPawn, Log, All);

AGridEnemyPawn::AGridEnemyPawn()
{
	PrimaryActorTick.bCanEverTick = false;

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
	return IsAlive() && GridManager != nullptr;
}

bool AGridEnemyPawn::TryMoveToGridCoord(FIntPoint TargetCoord)
{
	if (!CanAct())
	{
		return false;
	}

	if (!GridManager->RequestMove(this, CurrentGridCoord, TargetCoord))
	{
		return false;
	}

	CurrentGridCoord = TargetCoord;
	SetActorLocation(GridManager->GridToWorld(CurrentGridCoord));
	return true;
}

bool AGridEnemyPawn::ExecuteBasicTurn_Implementation(AGridPawn* PlayerPawn)
{
	if (!CanAct() || !PlayerPawn)
	{
		return false;
	}

	const FIntPoint PlayerCoord = PlayerPawn->CurrentGridCoord;
	const FIntPoint Delta = PlayerCoord - CurrentGridCoord;
	const int32 ManhattanDistance = FMath::Abs(Delta.X) + FMath::Abs(Delta.Y);
	if (ManhattanDistance == 1)
	{
		ExecuteMeleeAttack(PlayerPawn);
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
	UE_LOG(LogGridEnemyPawn, Log, TEXT("%s attacks %s."),
		*GetNameSafe(this), *GetNameSafe(PlayerPawn));
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
	UE_LOG(LogGridEnemyPawn, Log, TEXT("%s killed."), *GetNameSafe(this));
}
