#include "Enemy/GridEnemyPawn.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Grid/GridManager.h"

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
