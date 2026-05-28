#include "Player/GridPawn.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/TurnManager.h"
#include "Core/TurnStateTypes.h"
#include "EngineUtils.h"
#include "Grid/GridManager.h"
#include "Grid/TileEffectResolverComponent.h"
#include "Player/PlayerAttributeComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridPawn, Log, All);

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
	SetActorLocation(FMath::Lerp(VisualMoveFrom, VisualMoveTo, Alpha));

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
	if (!GridManager->IsValidCoord(TargetCoord) || !GridManager->IsWalkable(TargetCoord) || GridManager->IsOccupied(TargetCoord))
	{
		// Illegal movement is intentionally silent and does not consume a step.
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

void AGridPawn::StartVisualMove(const FVector& From, const FVector& To)
{
	VisualMoveFrom = From;
	VisualMoveTo = To;
	MoveElapsedTime = 0.f;
	bIsMoving = true;
	SetActorLocation(VisualMoveFrom);
	SetActorTickEnabled(true);
}

void AGridPawn::FinishVisualMove()
{
	// Snap at the end to prevent small interpolation drift from desyncing visuals and grid state.
	SetActorLocation(VisualMoveTo);
	bIsMoving = false;
	MoveElapsedTime = 0.f;
	SetActorTickEnabled(false);

	if (TurnManager)
	{
		TurnManager->EndPlayerAction();
	}
}
