#include "Player/GridPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CombatCameraDirectorComponent.h"
#include "Data/ChessPieceFormData.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Grid/GridManager.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Player/ConversionEnergyComponent.h"
#include "Player/GridPawn.h"
#include "Player/PlayerAttributeComponent.h"
#include "Player/PlayerTransformInventoryComponent.h"
#include "UI/PlayerAttributeHUDWidget.h"
#include "UI/TransformWheelWidget.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridPlayerController, Log, All);

AGridPlayerController::AGridPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

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

void AGridPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	UpdateEdgeScroll(DeltaTime);
	UpdateMouseHoverGridMaterialParameters();
	UpdatePendingTransformCameraRestore();
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
	if (OpenTransformWheelAction)
	{
		EnhancedInputComponent->BindAction(OpenTransformWheelAction, ETriggerEvent::Started, this, &AGridPlayerController::HandleTransformWheelStarted);
		EnhancedInputComponent->BindAction(OpenTransformWheelAction, ETriggerEvent::Completed, this, &AGridPlayerController::HandleTransformWheelReleased);
		EnhancedInputComponent->BindAction(OpenTransformWheelAction, ETriggerEvent::Canceled, this, &AGridPlayerController::HandleTransformWheelReleased);
	}
	if (TransformLeftClickAction)
	{
		EnhancedInputComponent->BindAction(TransformLeftClickAction, ETriggerEvent::Started, this, &AGridPlayerController::HandleTransformLeftClick);
	}
	if (TransformCancelAction)
	{
		EnhancedInputComponent->BindAction(TransformCancelAction, ETriggerEvent::Started, this, &AGridPlayerController::HandleTransformCancel);
	}
	if (TransformRightMouseAction)
	{
		EnhancedInputComponent->BindAction(TransformRightMouseAction, ETriggerEvent::Started, this, &AGridPlayerController::HandleTransformRightMouseStarted);
		EnhancedInputComponent->BindAction(TransformRightMouseAction, ETriggerEvent::Completed, this, &AGridPlayerController::HandleTransformRightMouseReleased);
		EnhancedInputComponent->BindAction(TransformRightMouseAction, ETriggerEvent::Canceled, this, &AGridPlayerController::HandleTransformRightMouseReleased);
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
	if (ControlMode != EPlayerControlMode::DefaultWASD)
	{
		return;
	}

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

void AGridPlayerController::HandleTransformWheelStarted()
{
	if (bTransformSelectionInProgress || ControlMode != EPlayerControlMode::DefaultWASD)
	{
		return;
	}

	AGridPawn* GridPawn = GetGridPawn();
	if (!GridPawn || !GridPawn->TransformInventoryComponent)
	{
		return;
	}

	ControlMode = EPlayerControlMode::TransformWheel;
	ShowTransformWheel();
	RefreshTransformWheel();

	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (TransformWheelWidget)
	{
		InputMode.SetWidgetToFocus(TransformWheelWidget->TakeWidget());
	}
	SetInputMode(InputMode);
}

void AGridPlayerController::HandleTransformWheelReleased()
{
	if (!bTransformSelectionInProgress && ControlMode == EPlayerControlMode::TransformWheel)
	{
		HideTransformWheel();
		ReturnToDefaultWASD();
	}
}

void AGridPlayerController::HandleTransformLeftClick()
{
	if (ControlMode != EPlayerControlMode::TransformTargeting || bIsCameraDragging)
	{
		return;
	}

	FIntPoint ClickedCoord;
	if (!TryGetGridCoordUnderMouse(ClickedCoord) || !HasPendingTransformTarget(ClickedCoord))
	{
		return;
	}

	AGridPawn* GridPawn = GetGridPawn();
	if (!GridPawn || !PendingTransformForm)
	{
		CancelTransformTargeting();
		return;
	}

	if (!GridPawn->TryTransformMoveToCoord(ClickedCoord, PendingTransformForm))
	{
		return;
	}

	WriteMouseHoverGridMaterialParameters(FIntPoint::ZeroValue, false);

	if (GridPawn->TransformInventoryComponent)
	{
		GridPawn->TransformInventoryComponent->ConsumeTransformPiece(PendingTransformForm->TransformType, 1);
	}

	FinishTransformTargeting();
}

void AGridPlayerController::HandleTransformCancel()
{
	if (ControlMode == EPlayerControlMode::TransformWheel)
	{
		HideTransformWheel();
		ReturnToDefaultWASD();
		return;
	}

	if (ControlMode == EPlayerControlMode::TransformTargeting)
	{
		CancelTransformTargeting();
	}
}

void AGridPlayerController::HandleTransformRightMouseStarted()
{
	if (ControlMode != EPlayerControlMode::TransformTargeting)
	{
		return;
	}

	bRightMousePressedForTransform = true;
	bIsCameraDragging = false;
	GetMousePosition(TransformRightMouseDownPosition.X, TransformRightMouseDownPosition.Y);
}

void AGridPlayerController::HandleTransformRightMouseReleased()
{
	if (!bRightMousePressedForTransform)
	{
		return;
	}

	bRightMousePressedForTransform = false;

	if (ControlMode != EPlayerControlMode::TransformTargeting)
	{
		bIsCameraDragging = false;
		return;
	}

	FVector2D MouseUpPosition = TransformRightMouseDownPosition;
	GetMousePosition(MouseUpPosition.X, MouseUpPosition.Y);
	const float DragDistance = FVector2D::Distance(TransformRightMouseDownPosition, MouseUpPosition);
	const bool bWasDrag = bIsCameraDragging || DragDistance >= RightMouseDragCancelThreshold;
	bIsCameraDragging = false;

	if (!bWasDrag)
	{
		CancelTransformTargeting();
	}
}

void AGridPlayerController::ShowTransformWheel()
{
	if (!IsLocalController() || !TransformWheelWidgetClass)
	{
		return;
	}

	if (!TransformWheelWidget)
	{
		TransformWheelWidget = CreateWidget<UTransformWheelWidget>(this, TransformWheelWidgetClass);
		if (TransformWheelWidget)
		{
			TransformWheelWidget->InitializeWheel(this);
		}
	}

	if (TransformWheelWidget && !TransformWheelWidget->IsInViewport())
	{
		TransformWheelWidget->AddToViewport(20);
	}
}

void AGridPlayerController::HideTransformWheel()
{
	if (TransformWheelWidget)
	{
		TransformWheelWidget->RemoveFromParent();
	}
}

void AGridPlayerController::RefreshTransformWheel()
{
	if (!TransformWheelWidget)
	{
		return;
	}

	AGridPawn* GridPawn = GetGridPawn();
	TransformWheelWidget->RefreshFromInventory(GridPawn ? GridPawn->TransformInventoryComponent : nullptr);
}

void AGridPlayerController::RequestSelectTransform(UChessPieceFormData* FormData)
{
	if (bTransformSelectionInProgress || ControlMode != EPlayerControlMode::TransformWheel || !FormData)
	{
		return;
	}

	bTransformSelectionInProgress = true;

	AGridPawn* GridPawn = GetGridPawn();
	if (!GridPawn || !GridPawn->TransformInventoryComponent
		|| !GridPawn->TransformInventoryComponent->CanConsumeTransformPiece(FormData->TransformType, 1))
	{
		bTransformSelectionInProgress = false;
		return;
	}

	PendingTransformTargets = GridPawn->BuildLegalTransformTargets(FormData);
	if (PendingTransformTargets.IsEmpty())
	{
		bTransformSelectionInProgress = false;
		return;
	}

	PendingTransformForm = FormData;
	ControlMode = EPlayerControlMode::TransformTargeting;
	bEdgeScrollEnabled = true;
	bIsCameraDragging = false;
	bShowMouseCursor = true;
	bRestoreCameraAfterTransformMove = false;

	HideTransformWheel();
	GridPawn->ApplyTransformVisual(FormData);

	if (CombatCameraDirectorComponent)
	{
		CombatCameraDirectorComponent->BeginTransformTargetingCameraSession();
	}

	ShowTransformTargetHighlights();

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	bTransformSelectionInProgress = false;
}

void AGridPlayerController::CancelTransformTargeting()
{
	if (AGridPawn* GridPawn = GetGridPawn())
	{
		GridPawn->RestoreDefaultPawnVisual();
	}

	ClearTransformTargetHighlights();
	PendingTransformForm = nullptr;
	PendingTransformTargets.Reset();
	bEdgeScrollEnabled = false;
	bIsCameraDragging = false;
	bTransformSelectionInProgress = false;
	bRestoreCameraAfterTransformMove = false;
	WriteMouseHoverGridMaterialParameters(FIntPoint::ZeroValue, false);
	if (CombatCameraDirectorComponent)
	{
		CombatCameraDirectorComponent->RestoreTransformTargetingCameraSession();
	}
	ReturnToDefaultWASD();
}

void AGridPlayerController::FinishTransformTargeting()
{
	ClearTransformTargetHighlights();
	PendingTransformForm = nullptr;
	PendingTransformTargets.Reset();
	bEdgeScrollEnabled = false;
	bIsCameraDragging = false;
	bTransformSelectionInProgress = false;
	bRestoreCameraAfterTransformMove = true;
	WriteMouseHoverGridMaterialParameters(FIntPoint::ZeroValue, false);
	ReturnToDefaultWASD();
}

void AGridPlayerController::ReturnToDefaultWASD()
{
	ControlMode = EPlayerControlMode::DefaultWASD;
	bEdgeScrollEnabled = false;
	bIsCameraDragging = false;
	bTransformSelectionInProgress = false;
	bShowMouseCursor = false;
	WriteMouseHoverGridMaterialParameters(FIntPoint::ZeroValue, false);

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	if (AGridPawn* GridPawn = GetGridPawn())
	{
		GridPawn->RefreshPlayerNextMoveTiles();
	}
}

void AGridPlayerController::ShowTransformTargetHighlights()
{
	AGridPawn* GridPawn = GetGridPawn();
	if (!GridPawn || !GridPawn->GridManager)
	{
		return;
	}

	TArray<FIntPoint> HighlightCoords;
	HighlightCoords.Reserve(PendingTransformTargets.Num());
	for (const FTransformMoveTarget& Target : PendingTransformTargets)
	{
		HighlightCoords.Add(Target.Coord);
	}

	GridPawn->GridManager->SetPlayerNextMoveTiles(HighlightCoords);
}

void AGridPlayerController::ClearTransformTargetHighlights()
{
	if (AGridPawn* GridPawn = GetGridPawn())
	{
		if (GridPawn->GridManager)
		{
			GridPawn->GridManager->ClearPlayerNextMoveTiles();
		}
	}
}

void AGridPlayerController::UpdateEdgeScroll(float DeltaTime)
{
	if (ControlMode != EPlayerControlMode::TransformTargeting || !bEdgeScrollEnabled)
	{
		return;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	if (bRightMousePressedForTransform)
	{
		const FVector2D CurrentMousePosition(MouseX, MouseY);
		if (FVector2D::Distance(TransformRightMouseDownPosition, CurrentMousePosition) >= RightMouseDragCancelThreshold)
		{
			bIsCameraDragging = true;
		}
	}

	if (bIsCameraDragging)
	{
		return;
	}

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	GetViewportSize(ViewportX, ViewportY);
	if (ViewportX <= 0 || ViewportY <= 0)
	{
		return;
	}

	FVector2D ScrollDirection = FVector2D::ZeroVector;
	if (MouseX <= EdgeScrollZoneSize)
	{
		ScrollDirection.X -= 1.f;
	}
	else if (MouseX >= static_cast<float>(ViewportX) - EdgeScrollZoneSize)
	{
		ScrollDirection.X += 1.f;
	}

	if (MouseY <= EdgeScrollZoneSize)
	{
		ScrollDirection.Y += 1.f;
	}
	else if (MouseY >= static_cast<float>(ViewportY) - EdgeScrollZoneSize)
	{
		ScrollDirection.Y -= 1.f;
	}

	if (!ScrollDirection.IsNearlyZero())
	{
		ScrollDirection.Normalize();
		ApplyCameraEdgeScroll(ScrollDirection, DeltaTime);
	}
}

void AGridPlayerController::UpdatePendingTransformCameraRestore()
{
	if (!bRestoreCameraAfterTransformMove)
	{
		return;
	}

	const AGridPawn* GridPawn = GetGridPawn();
	if (!GridPawn || GridPawn->bIsMoving)
	{
		return;
	}

	bRestoreCameraAfterTransformMove = false;
	if (CombatCameraDirectorComponent)
	{
		CombatCameraDirectorComponent->RestoreTransformTargetingCameraSession();
	}
}

void AGridPlayerController::ApplyCameraEdgeScroll(const FVector2D& ScreenDirection, float DeltaTime)
{
	if (CombatCameraDirectorComponent)
	{
		AGridPawn* GridPawn = GetGridPawn();
		CombatCameraDirectorComponent->PanCameraByScreenDirection(
			ScreenDirection,
			EdgeScrollSpeed * DeltaTime,
			GridPawn ? GridPawn->GridManager : nullptr,
			EdgeScrollGridPaddingTiles);
	}
}

void AGridPlayerController::UpdateMouseHoverGridMaterialParameters()
{
	if (bWriteMouseHoverGridOnlyWhileTransformTargeting && ControlMode != EPlayerControlMode::TransformTargeting)
	{
		WriteMouseHoverGridMaterialParameters(FIntPoint::ZeroValue, false);
		return;
	}

	if (bIsCameraDragging)
	{
		WriteMouseHoverGridMaterialParameters(FIntPoint::ZeroValue, false);
		return;
	}

	FIntPoint HoverCoord;
	if (!TryGetGridCoordUnderMouse(HoverCoord))
	{
		WriteMouseHoverGridMaterialParameters(FIntPoint::ZeroValue, false);
		return;
	}

	if (bMouseHoverRequiresPendingTransformTarget && !HasPendingTransformTarget(HoverCoord))
	{
		WriteMouseHoverGridMaterialParameters(HoverCoord, false);
		return;
	}

	WriteMouseHoverGridMaterialParameters(HoverCoord, true);
}

void AGridPlayerController::WriteMouseHoverGridMaterialParameters(const FIntPoint& HoverCoord, bool bHasHover)
{
	if (bHasWrittenMouseHoverGridParameters
		&& bLastWrittenMouseHoverGridHasHover == bHasHover
		&& LastWrittenMouseHoverGridCoord == HoverCoord)
	{
		return;
	}

	UMaterialParameterCollection* ParameterCollection = GetMouseHoverGridParameterCollection();
	if (!ParameterCollection)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UMaterialParameterCollectionInstance* CollectionInstance =
		World->GetParameterCollectionInstance(ParameterCollection);
	if (!CollectionInstance)
	{
		UE_LOG(LogGridPlayerController, Warning, TEXT("WriteMouseHoverGridMaterialParameters failed: collection instance is unavailable."));
		return;
	}

	CollectionInstance->SetScalarParameterValue(MouseHoverGridXParameterName, static_cast<float>(HoverCoord.X));
	CollectionInstance->SetScalarParameterValue(MouseHoverGridYParameterName, static_cast<float>(HoverCoord.Y));
	CollectionInstance->SetScalarParameterValue(HasMouseHoverGridParameterName, bHasHover ? 1.f : 0.f);

	LastWrittenMouseHoverGridCoord = HoverCoord;
	bLastWrittenMouseHoverGridHasHover = bHasHover;
	bHasWrittenMouseHoverGridParameters = true;
}

UMaterialParameterCollection* AGridPlayerController::GetMouseHoverGridParameterCollection() const
{
	if (MouseHoverGridParameterCollection)
	{
		return MouseHoverGridParameterCollection;
	}

	const AGridPawn* GridPawn = GetGridPawn();
	return GridPawn ? GridPawn->PlayerGridParameterCollection : nullptr;
}

bool AGridPlayerController::TryGetGridCoordUnderMouse(FIntPoint& OutGridCoord) const
{
	const AGridPawn* GridPawn = GetGridPawn();
	if (!GridPawn || !GridPawn->GridManager)
	{
		return false;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	const FVector PlanePoint = GridPawn->GridManager->GridToWorld(GridPawn->CurrentGridCoord);
	const float T = (PlanePoint.Z - WorldOrigin.Z) / WorldDirection.Z;
	if (T < 0.f)
	{
		return false;
	}

	const FVector HitWorldLocation = WorldOrigin + WorldDirection * T;
	const FIntPoint HitCoord = GridPawn->GridManager->WorldToGrid(HitWorldLocation);
	if (!GridPawn->GridManager->IsValidCoord(HitCoord))
	{
		return false;
	}

	OutGridCoord = HitCoord;
	return true;
}

bool AGridPlayerController::HasPendingTransformTarget(FIntPoint Coord) const
{
	return PendingTransformTargets.ContainsByPredicate(
		[Coord](const FTransformMoveTarget& Target)
		{
			return Target.Coord == Coord;
		});
}

AGridPawn* AGridPlayerController::GetGridPawn() const
{
	return Cast<AGridPawn>(GetPawn());
}

void AGridPlayerController::NotifyTransformCameraDragStarted()
{
	if (ControlMode == EPlayerControlMode::TransformTargeting)
	{
		bIsCameraDragging = true;
	}
}

void AGridPlayerController::NotifyTransformCameraDragEnded()
{
	bIsCameraDragging = false;
}

EPlayerControlMode AGridPlayerController::GetPlayerControlMode() const
{
	return ControlMode;
}

TArray<FTransformMoveTarget> AGridPlayerController::GetPendingTransformTargets() const
{
	return PendingTransformTargets;
}

TArray<UChessPieceFormData*> AGridPlayerController::GetTransformWheelForms() const
{
	TArray<UChessPieceFormData*> Forms;
	Forms.Reserve(TransformWheelForms.Num());
	for (UChessPieceFormData* Form : TransformWheelForms)
	{
		if (Form)
		{
			Forms.Add(Form);
		}
	}

	return Forms;
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
