#include "Player/GridPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Player/GridPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridPlayerController, Log, All);

void AGridPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!GridMovementMappingContext)
	{
		UE_LOG(LogGridPlayerController, Warning, TEXT("GridMovementMappingContext is not assigned."));
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			Subsystem->AddMappingContext(GridMovementMappingContext, 0);
		}
	}
}

void AGridPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogGridPlayerController, Warning, TEXT("GridPlayerController requires an EnhancedInputComponent."));
		return;
	}

	if (MoveUpAction)
	{
		EnhancedInputComponent->BindAction(MoveUpAction, ETriggerEvent::Started, this, &AGridPlayerController::MoveUp);
	}
	if (MoveDownAction)
	{
		EnhancedInputComponent->BindAction(MoveDownAction, ETriggerEvent::Started, this, &AGridPlayerController::MoveDown);
	}
	if (MoveLeftAction)
	{
		EnhancedInputComponent->BindAction(MoveLeftAction, ETriggerEvent::Started, this, &AGridPlayerController::MoveLeft);
	}
	if (MoveRightAction)
	{
		EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Started, this, &AGridPlayerController::MoveRight);
	}
}

void AGridPlayerController::MoveUp()
{
	RequestPawnMove(FIntPoint(0, 1));
}

void AGridPlayerController::MoveDown()
{
	RequestPawnMove(FIntPoint(0, -1));
}

void AGridPlayerController::MoveLeft()
{
	RequestPawnMove(FIntPoint(1, 0));
}

void AGridPlayerController::MoveRight()
{
	RequestPawnMove(FIntPoint(-1, 0));
}

void AGridPlayerController::RequestPawnMove(FIntPoint Direction)
{
	AGridPawn* GridPawn = Cast<AGridPawn>(GetPawn());
	if (!GridPawn)
	{
		UE_LOG(LogGridPlayerController, Warning, TEXT("Move input ignored: controlled pawn is not AGridPawn."));
		return;
	}

	GridPawn->TryMove(Direction);
}
