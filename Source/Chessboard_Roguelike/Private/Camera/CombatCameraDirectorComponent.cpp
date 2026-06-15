#include "Camera/CombatCameraDirectorComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Grid/GridManager.h"
#include "Grid/GridSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatCameraDirector, Log, All);

UCombatCameraDirectorComponent::UCombatCameraDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCombatCameraDirectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	USpringArmComponent* SpringArm = ActiveSpringArm.Get();
	UWorld* World = GetWorld();
	if (!bFocusActive || !SpringArm || !World)
	{
		if (!bConversionEnergyZoomActive || !SpringArm || !World)
		{
			StopFocus();
			EndConversionEnergyZoom();
			return;
		}
	}

	if (!SpringArm || !World)
	{
		StopFocus();
		EndConversionEnergyZoom();
		return;
	}

	const double CurrentRealTimeSeconds = World->GetRealTimeSeconds();
	const float RealDeltaSeconds = LastRealTimeSeconds > 0.0
		? static_cast<float>(CurrentRealTimeSeconds - LastRealTimeSeconds)
		: DeltaTime;
	LastRealTimeSeconds = CurrentRealTimeSeconds;

	const float SafeRealDeltaSeconds = FMath::Max(0.f, RealDeltaSeconds);

	if (bFocusActive)
	{
		TickDeathFocus(SafeRealDeltaSeconds);
		return;
	}

	if (bConversionEnergyZoomActive)
	{
		TickConversionEnergyZoom(SafeRealDeltaSeconds);
	}
}

void UCombatCameraDirectorComponent::FocusGridTileBriefly(const FVector& TargetWorldLocation)
{
	USpringArmComponent* SpringArm = FindSpringArm();
	UWorld* World = GetWorld();
	if (!SpringArm || !World)
	{
		UE_LOG(LogCombatCameraDirector, Verbose, TEXT("FocusGridTileBriefly skipped: no SpringArmComponent found."));
		return;
	}

	ActiveSpringArm = SpringArm;
	if (bConversionEnergyZoomActive)
	{
		SpringArm->TargetArmLength = ConversionEnergyRestArmLength;
		RestoreSpringArmLag(SpringArm);
		ResetConversionEnergyZoomState();
	}

	if (!bFocusActive)
	{
		RestTargetOffset = SpringArm->TargetOffset;
		RestArmLength = SpringArm->TargetArmLength;
	}

	StartTargetOffset = SpringArm->TargetOffset;
	StartArmLength = SpringArm->TargetArmLength;
	FocusWorldLocation = TargetWorldLocation;
	FocusTargetOffset = CalculateFocusTargetOffset(SpringArm, TargetWorldLocation);
	FocusArmLength = FMath::Max(0.f, RestArmLength - ZoomInDistance);

	RuntimeFocusInDuration = FocusInDuration;
	RuntimeFocusHoldDuration = FocusHoldDuration;
	RuntimeFocusOutDuration = FocusOutDuration;

	ElapsedRealTime = 0.f;
	LastRealTimeSeconds = World->GetRealTimeSeconds();
	bFocusActive = true;

	CacheAndDisableSpringArmLag(SpringArm);
	SetComponentTickEnabled(true);
}

void UCombatCameraDirectorComponent::StopFocus()
{
	if (USpringArmComponent* SpringArm = ActiveSpringArm.Get())
	{
		SpringArm->TargetOffset = RestTargetOffset;
		SpringArm->TargetArmLength = RestArmLength;
		RestoreSpringArmLag(SpringArm);
	}

	RuntimeFocusInDuration = 0.f;
	RuntimeFocusHoldDuration = 0.f;
	RuntimeFocusOutDuration = 0.f;
	FocusWorldLocation = FVector::ZeroVector;
	ElapsedRealTime = 0.f;
	LastRealTimeSeconds = 0.0;
	bFocusActive = false;
	bHasCachedSpringArmLagSettings = false;
	ActiveSpringArm.Reset();
	SetComponentTickEnabled(false);
}

void UCombatCameraDirectorComponent::BeginConversionEnergyZoom()
{
	USpringArmComponent* SpringArm = FindSpringArm();
	UWorld* World = GetWorld();
	if (!SpringArm || !World)
	{
		UE_LOG(LogCombatCameraDirector, Verbose, TEXT("BeginConversionEnergyZoom skipped: no SpringArmComponent found."));
		return;
	}

	if (bFocusActive)
	{
		StopFocus();
	}

	ActiveSpringArm = SpringArm;
	if (!bConversionEnergyZoomActive)
	{
		ConversionEnergyRestArmLength = SpringArm->TargetArmLength;
	}

	ConversionEnergyStartArmLength = SpringArm->TargetArmLength;
	ConversionEnergyTargetArmLength = FMath::Max(0.f, ConversionEnergyRestArmLength - ConversionEnergyZoomInDistance);
	ConversionEnergyElapsedRealTime = 0.f;
	LastRealTimeSeconds = World->GetRealTimeSeconds();
	bConversionEnergyZoomActive = true;
	bConversionEnergyZoomReturning = false;

	if (bDisableSpringArmLagDuringConversionEnergyZoom)
	{
		CacheAndDisableSpringArmLag(SpringArm);
	}

	SetComponentTickEnabled(true);
}

void UCombatCameraDirectorComponent::EndConversionEnergyZoom()
{
	if (!bConversionEnergyZoomActive)
	{
		return;
	}

	if (bConversionEnergyZoomReturning)
	{
		return;
	}

	USpringArmComponent* SpringArm = ActiveSpringArm.Get();
	UWorld* World = GetWorld();
	if (!SpringArm || !World)
	{
		ResetConversionEnergyZoomState();
		return;
	}

	ConversionEnergyStartArmLength = SpringArm->TargetArmLength;
	ConversionEnergyElapsedRealTime = 0.f;
	LastRealTimeSeconds = World->GetRealTimeSeconds();
	bConversionEnergyZoomReturning = true;

	SetComponentTickEnabled(true);
}

void UCombatCameraDirectorComponent::PanCameraByScreenDirection(
	FVector2D ScreenDirection,
	float Distance,
	AGridManager* GridManager,
	float GridPaddingTiles)
{
	if (ScreenDirection.IsNearlyZero() || Distance <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	USpringArmComponent* SpringArm = FindSpringArm();
	if (!SpringArm)
	{
		return;
	}

	ScreenDirection.Normalize();
	FVector Right = SpringArm->GetRightVector();
	FVector Forward = SpringArm->GetForwardVector();
	Right.Z = 0.f;
	Forward.Z = 0.f;

	if (!Right.Normalize() || !Forward.Normalize())
	{
		return;
	}

	const FVector WorldDelta = (Right * ScreenDirection.X + Forward * ScreenDirection.Y).GetSafeNormal() * Distance;
	const FVector DesiredTargetOffset = SpringArm->TargetOffset + WorldDelta;
	SpringArm->TargetOffset = ClampTargetOffsetToGridBounds(SpringArm, DesiredTargetOffset, GridManager, GridPaddingTiles);
}

void UCombatCameraDirectorComponent::BeginTransformTargetingCameraSession()
{
	USpringArmComponent* SpringArm = FindSpringArm();
	if (!SpringArm)
	{
		return;
	}

	TransformTargetingRestTargetOffset = SpringArm->TargetOffset;
	bHasTransformTargetingRestTargetOffset = true;
}

void UCombatCameraDirectorComponent::RestoreTransformTargetingCameraSession()
{
	USpringArmComponent* SpringArm = FindSpringArm();
	if (!SpringArm)
	{
		bHasTransformTargetingRestTargetOffset = false;
		return;
	}

	if (bFocusActive)
	{
		StopFocus();
		SpringArm = FindSpringArm();
		if (!SpringArm)
		{
			bHasTransformTargetingRestTargetOffset = false;
			return;
		}
	}

	SpringArm->TargetOffset = bHasTransformTargetingRestTargetOffset
		? TransformTargetingRestTargetOffset
		: FVector::ZeroVector;

	bHasTransformTargetingRestTargetOffset = false;
}

USpringArmComponent* UCombatCameraDirectorComponent::FindSpringArm()
{
	AActor* OwnerActor = GetOwner();
	APlayerController* PlayerController = Cast<APlayerController>(OwnerActor);
	if (PlayerController)
	{
		if (AActor* ViewTarget = PlayerController->GetViewTarget())
		{
			if (USpringArmComponent* SpringArm = ViewTarget->FindComponentByClass<USpringArmComponent>())
			{
				return SpringArm;
			}
		}

		if (APawn* Pawn = PlayerController->GetPawn())
		{
			if (USpringArmComponent* SpringArm = Pawn->FindComponentByClass<USpringArmComponent>())
			{
				return SpringArm;
			}
		}
	}

	return OwnerActor ? OwnerActor->FindComponentByClass<USpringArmComponent>() : nullptr;
}

FVector UCombatCameraDirectorComponent::CalculateFocusTargetOffset(
	const USpringArmComponent* SpringArm,
	const FVector& TargetWorldLocation) const
{
	if (!SpringArm)
	{
		return RestTargetOffset;
	}

	return TargetWorldLocation - SpringArm->GetComponentLocation();
}

float UCombatCameraDirectorComponent::GetTotalDuration() const
{
	return RuntimeFocusInDuration + RuntimeFocusHoldDuration + RuntimeFocusOutDuration;
}

void UCombatCameraDirectorComponent::ApplyFocusState(float Alpha)
{
	USpringArmComponent* SpringArm = ActiveSpringArm.Get();
	if (!SpringArm)
	{
		return;
	}

	const float InEnd = RuntimeFocusInDuration;
	const float HoldEnd = RuntimeFocusInDuration + RuntimeFocusHoldDuration;
	const float TotalDuration = GetTotalDuration();
	const float CurrentTime = Alpha * TotalDuration;
	const FVector CurrentFocusTargetOffset = CalculateFocusTargetOffset(SpringArm, FocusWorldLocation);

	FVector NewTargetOffset = CurrentFocusTargetOffset;
	float NewArmLength = FocusArmLength;

	if (RuntimeFocusInDuration > KINDA_SMALL_NUMBER && CurrentTime < InEnd)
	{
		float BlendAlpha = FMath::Clamp(CurrentTime / RuntimeFocusInDuration, 0.f, 1.f);
		BlendAlpha = BlendAlpha * BlendAlpha * (3.f - 2.f * BlendAlpha);
		NewTargetOffset = FMath::Lerp(StartTargetOffset, CurrentFocusTargetOffset, BlendAlpha);
		NewArmLength = FMath::Lerp(StartArmLength, FocusArmLength, BlendAlpha);
	}
	else if (RuntimeFocusOutDuration > KINDA_SMALL_NUMBER && CurrentTime > HoldEnd)
	{
		float OutAlpha = FMath::Clamp((CurrentTime - HoldEnd) / RuntimeFocusOutDuration, 0.f, 1.f);
		OutAlpha = OutAlpha * OutAlpha * (3.f - 2.f * OutAlpha);
		NewTargetOffset = FMath::Lerp(CurrentFocusTargetOffset, RestTargetOffset, OutAlpha);
		NewArmLength = FMath::Lerp(FocusArmLength, RestArmLength, OutAlpha);
	}

	SpringArm->TargetOffset = NewTargetOffset;
	SpringArm->TargetArmLength = NewArmLength;
}

void UCombatCameraDirectorComponent::TickDeathFocus(float RealDeltaSeconds)
{
	ElapsedRealTime += RealDeltaSeconds;

	const float TotalDuration = GetTotalDuration();
	if (TotalDuration <= KINDA_SMALL_NUMBER)
	{
		StopFocus();
		return;
	}

	ApplyFocusState(FMath::Clamp(ElapsedRealTime / TotalDuration, 0.f, 1.f));

	if (ElapsedRealTime >= TotalDuration)
	{
		StopFocus();
	}
}

void UCombatCameraDirectorComponent::TickConversionEnergyZoom(float RealDeltaSeconds)
{
	USpringArmComponent* SpringArm = ActiveSpringArm.Get();
	if (!SpringArm)
	{
		ResetConversionEnergyZoomState();
		return;
	}

	const float Duration = bConversionEnergyZoomReturning
		? ConversionEnergyZoomOutDuration
		: ConversionEnergyZoomInDuration;
	ConversionEnergyElapsedRealTime += RealDeltaSeconds;

	const float Alpha = Duration <= KINDA_SMALL_NUMBER
		? 1.f
		: FMath::Clamp(ConversionEnergyElapsedRealTime / Duration, 0.f, 1.f);
	const float SmoothAlpha = Alpha * Alpha * (3.f - 2.f * Alpha);
	const float TargetArmLength = bConversionEnergyZoomReturning
		? ConversionEnergyRestArmLength
		: ConversionEnergyTargetArmLength;

	SpringArm->TargetArmLength = FMath::Lerp(ConversionEnergyStartArmLength, TargetArmLength, SmoothAlpha);

	if (Alpha < 1.f)
	{
		return;
	}

	SpringArm->TargetArmLength = TargetArmLength;
	if (bConversionEnergyZoomReturning)
	{
		RestoreSpringArmLag(SpringArm);
		ResetConversionEnergyZoomState();
	}
}

void UCombatCameraDirectorComponent::CacheAndDisableSpringArmLag(USpringArmComponent* SpringArm)
{
	if (!SpringArm || !bDisableSpringArmLagDuringFocus)
	{
		return;
	}

	if (!bHasCachedSpringArmLagSettings)
	{
		bRestCameraLagEnabled = SpringArm->bEnableCameraLag;
		bRestCameraRotationLagEnabled = SpringArm->bEnableCameraRotationLag;
		RestCameraLagSpeed = SpringArm->CameraLagSpeed;
		RestCameraRotationLagSpeed = SpringArm->CameraRotationLagSpeed;
		bHasCachedSpringArmLagSettings = true;
	}

	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = false;
}

void UCombatCameraDirectorComponent::RestoreSpringArmLag(USpringArmComponent* SpringArm)
{
	if (!SpringArm || !bHasCachedSpringArmLagSettings)
	{
		return;
	}

	SpringArm->bEnableCameraLag = bRestCameraLagEnabled;
	SpringArm->bEnableCameraRotationLag = bRestCameraRotationLagEnabled;
	SpringArm->CameraLagSpeed = RestCameraLagSpeed;
	SpringArm->CameraRotationLagSpeed = RestCameraRotationLagSpeed;
}

void UCombatCameraDirectorComponent::ResetConversionEnergyZoomState()
{
	ConversionEnergyRestArmLength = 0.f;
	ConversionEnergyStartArmLength = 0.f;
	ConversionEnergyTargetArmLength = 0.f;
	ConversionEnergyElapsedRealTime = 0.f;
	bConversionEnergyZoomActive = false;
	bConversionEnergyZoomReturning = false;

	if (!bFocusActive)
	{
		bHasCachedSpringArmLagSettings = false;
		ActiveSpringArm.Reset();
		SetComponentTickEnabled(false);
	}
}

FVector UCombatCameraDirectorComponent::ClampTargetOffsetToGridBounds(
	const USpringArmComponent* SpringArm,
	const FVector& DesiredTargetOffset,
	const AGridManager* GridManager,
	float GridPaddingTiles) const
{
	if (!SpringArm || !GridManager || !GridManager->GridSettings || GridManager->CurrentWidth <= 0 || GridManager->CurrentHeight <= 0)
	{
		return DesiredTargetOffset;
	}

	const float TileSize = GridManager->GridSettings->TileSize;
	if (TileSize <= KINDA_SMALL_NUMBER)
	{
		return DesiredTargetOffset;
	}

	const float Padding = FMath::Max(0.f, GridPaddingTiles) * TileSize;
	const FVector MinWorld = GridManager->GridToWorld(FIntPoint(0, 0)) - FVector(Padding, Padding, 0.f);
	const FVector MaxWorld = GridManager->GridToWorld(FIntPoint(GridManager->CurrentWidth - 1, GridManager->CurrentHeight - 1)) + FVector(Padding, Padding, 0.f);

	FVector DesiredFocusWorld = SpringArm->GetComponentLocation() + DesiredTargetOffset;
	DesiredFocusWorld.X = FMath::Clamp(DesiredFocusWorld.X, FMath::Min(MinWorld.X, MaxWorld.X), FMath::Max(MinWorld.X, MaxWorld.X));
	DesiredFocusWorld.Y = FMath::Clamp(DesiredFocusWorld.Y, FMath::Min(MinWorld.Y, MaxWorld.Y), FMath::Max(MinWorld.Y, MaxWorld.Y));

	FVector ClampedTargetOffset = DesiredFocusWorld - SpringArm->GetComponentLocation();
	ClampedTargetOffset.Z = DesiredTargetOffset.Z;
	return ClampedTargetOffset;
}
