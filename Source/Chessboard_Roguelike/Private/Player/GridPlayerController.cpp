#include "Player/GridPlayerController.h"

#include "Camera/CombatCameraDirectorComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Player/ConversionEnergyComponent.h"
#include "Player/GridPawn.h"
#include "Player/PlayerAttributeComponent.h"
#include "UI/PlayerAttributeHUDWidget.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridPlayerController, Log, All);

AGridPlayerController::AGridPlayerController()
{
	CombatCameraDirectorComponent = CreateDefaultSubobject<UCombatCameraDirectorComponent>(TEXT("CombatCameraDirectorComponent"));

	PlayerAttributeHUDClass = UPlayerAttributeHUDWidget::StaticClass();

	// Prefer the authored WBP asset when present, with the native widget as a safe fallback.
	static ConstructorHelpers::FClassFinder<UPlayerAttributeHUDWidget> PlayerAttributeHUDWidgetClass(
		TEXT("/Game/UI/WBP_PlayerAttributeHUD"));
	if (PlayerAttributeHUDWidgetClass.Succeeded())
	{
		PlayerAttributeHUDClass = PlayerAttributeHUDWidgetClass.Class;
	}
}

void AGridPlayerController::FocusCombatCameraOnGridTile(const FVector& TargetWorldLocation)
{
	if (IsLocalController() && CombatCameraDirectorComponent)
	{
		CombatCameraDirectorComponent->FocusGridTileBriefly(TargetWorldLocation);
	}
}

void AGridPlayerController::BeginConversionEnergyCameraZoom()
{
	if (IsLocalController() && CombatCameraDirectorComponent)
	{
		CombatCameraDirectorComponent->BeginConversionEnergyZoom();
	}
}

void AGridPlayerController::EndConversionEnergyCameraZoom()
{
	if (IsLocalController() && CombatCameraDirectorComponent)
	{
		CombatCameraDirectorComponent->EndConversionEnergyZoom();
	}
}

bool AGridPlayerController::CanStartConversionEnergyCameraZoom_Implementation() const
{
	const AGridPawn* GridPawn = Cast<AGridPawn>(GetPawn());
	return GridPawn && GridPawn->ConversionEnergyComponent && GridPawn->ConversionEnergyComponent->HasConversionEnergy();
}

void AGridPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && PlayerAttributeHUDClass)
	{
		// HUD is read-only and updates by subscribing to the pawn's attribute component.
		PlayerAttributeHUD = CreateWidget<UPlayerAttributeHUDWidget>(this, PlayerAttributeHUDClass);
		if (PlayerAttributeHUD)
		{
			if (APawn* ControlledPawn = GetPawn())
			{
				PlayerAttributeHUD->InitializeFromAttributeComponent(ControlledPawn->FindComponentByClass<UPlayerAttributeComponent>());
				PlayerAttributeHUD->InitializeFromConversionEnergyComponent(ControlledPawn->FindComponentByClass<UConversionEnergyComponent>());
			}
			PlayerAttributeHUD->AddToViewport();
		}
	}

	if (!GridMovementMappingContext)
	{
		UE_LOG(LogGridPlayerController, Warning, TEXT("GridMovementMappingContext is not assigned."));
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			// Context is added once on BeginPlay; each key press is handled by action Started events.
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
		// Started behaves like a single press for Boolean actions and still respects pawn movement locks.
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
	if (UseEnergyAction)
	{
		EnhancedInputComponent->BindAction(UseEnergyAction, ETriggerEvent::Started, this, &AGridPlayerController::HandleUseEnergyStarted);
		EnhancedInputComponent->BindAction(UseEnergyAction, ETriggerEvent::Triggered, this, &AGridPlayerController::HandleUseEnergyFinished);
		EnhancedInputComponent->BindAction(UseEnergyAction, ETriggerEvent::Completed, this, &AGridPlayerController::HandleUseEnergyFinished);
		EnhancedInputComponent->BindAction(UseEnergyAction, ETriggerEvent::Canceled, this, &AGridPlayerController::HandleUseEnergyFinished);
	}
	if (SwitchEnergyTypeAction)
	{
		EnhancedInputComponent->BindAction(SwitchEnergyTypeAction, ETriggerEvent::Started, this, &AGridPlayerController::HandleSwitchEnergyType);
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

	// Controller translates input into grid directions; the pawn/GridManager validate the move.
	GridPawn->TryMove(Direction);
}

void AGridPlayerController::HandleUseEnergyStarted()
{
	if (CanStartConversionEnergyCameraZoom())
	{
		BeginConversionEnergyCameraZoom();
	}
}

void AGridPlayerController::HandleUseEnergyFinished()
{
	EndConversionEnergyCameraZoom();
}

void AGridPlayerController::HandleSwitchEnergyType()
{
	AGridPawn* GridPawn = Cast<AGridPawn>(GetPawn());
	if (GridPawn && GridPawn->ConversionEnergyComponent)
	{
		GridPawn->ConversionEnergyComponent->CycleHeldConversionEnergyType();
	}
}
