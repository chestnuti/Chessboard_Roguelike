#include "Camera/CombatCameraDirectorComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

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
	FocusTargetOffset = CalculateFocusTargetOffset(SpringArm, TargetWorldLocation);
	FocusArmLength = FMath::Max(0.f, RestArmLength - ZoomInDistance);

	const float DistanceAlpha = CalculateFocusDistanceAlpha(SpringArm, TargetWorldLocation);
	RuntimeFocusInDuration = FocusInDuration + ExtraFocusInDurationAtMaxDistance * DistanceAlpha;
	RuntimeFocusHoldDuration = FocusHoldDuration + ExtraFocusHoldDurationAtMaxDistance * DistanceAlpha;
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
	const AActor* OwnerActor = GetOwner();
	const APlayerController* PlayerController = Cast<APlayerController>(OwnerActor);
	const AActor* OriginActor = PlayerController ? PlayerController->GetPawn() : OwnerActor;
	const FVector OriginLocation = OriginActor ? OriginActor->GetActorLocation() : SpringArm->GetComponentLocation();

	FVector PlanarDelta = TargetWorldLocation - OriginLocation;
	PlanarDelta.Z = 0.f;

	if (PlanarDelta.SizeSquared() > FMath::Square(MaxFocusOffset))
	{
		PlanarDelta = PlanarDelta.GetClampedToMaxSize(MaxFocusOffset);
	}

	return RestTargetOffset + PlanarDelta * FocusOffsetScale;
}

float UCombatCameraDirectorComponent::CalculateFocusDistanceAlpha(
	const USpringArmComponent* SpringArm,
	const FVector& TargetWorldLocation) const
{
	if (!SpringArm || MaxFocusOffset <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	const AActor* OwnerActor = GetOwner();
	const APlayerController* PlayerController = Cast<APlayerController>(OwnerActor);
	const AActor* OriginActor = PlayerController ? PlayerController->GetPawn() : OwnerActor;
	const FVector OriginLocation = OriginActor ? OriginActor->GetActorLocation() : SpringArm->GetComponentLocation();

	return FMath::Clamp(FVector::Dist2D(OriginLocation, TargetWorldLocation) / MaxFocusOffset, 0.f, 1.f);
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

	FVector NewTargetOffset = FocusTargetOffset;
	float NewArmLength = FocusArmLength;

	if (RuntimeFocusInDuration > KINDA_SMALL_NUMBER && CurrentTime < InEnd)
	{
		float BlendAlpha = FMath::Clamp(CurrentTime / RuntimeFocusInDuration, 0.f, 1.f);
		BlendAlpha = BlendAlpha * BlendAlpha * (3.f - 2.f * BlendAlpha);
		NewTargetOffset = FMath::Lerp(StartTargetOffset, FocusTargetOffset, BlendAlpha);
		NewArmLength = FMath::Lerp(StartArmLength, FocusArmLength, BlendAlpha);
	}
	else if (RuntimeFocusOutDuration > KINDA_SMALL_NUMBER && CurrentTime > HoldEnd)
	{
		float OutAlpha = FMath::Clamp((CurrentTime - HoldEnd) / RuntimeFocusOutDuration, 0.f, 1.f);
		OutAlpha = OutAlpha * OutAlpha * (3.f - 2.f * OutAlpha);
		NewTargetOffset = FMath::Lerp(FocusTargetOffset, RestTargetOffset, OutAlpha);
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
