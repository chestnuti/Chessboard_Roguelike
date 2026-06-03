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
		StopFocus();
		return;
	}

	const double CurrentRealTimeSeconds = World->GetRealTimeSeconds();
	const float RealDeltaSeconds = LastRealTimeSeconds > 0.0
		? static_cast<float>(CurrentRealTimeSeconds - LastRealTimeSeconds)
		: DeltaTime;
	LastRealTimeSeconds = CurrentRealTimeSeconds;

	ElapsedRealTime += FMath::Max(0.f, RealDeltaSeconds);

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
	if (!bFocusActive)
	{
		RestTargetOffset = SpringArm->TargetOffset;
		RestArmLength = SpringArm->TargetArmLength;
	}

	StartTargetOffset = SpringArm->TargetOffset;
	StartArmLength = SpringArm->TargetArmLength;
	FocusTargetOffset = CalculateFocusTargetOffset(SpringArm, TargetWorldLocation);
	FocusArmLength = FMath::Max(0.f, RestArmLength - ZoomInDistance);
	ElapsedRealTime = 0.f;
	LastRealTimeSeconds = World->GetRealTimeSeconds();
	bFocusActive = true;

	SetComponentTickEnabled(true);
}

void UCombatCameraDirectorComponent::StopFocus()
{
	if (USpringArmComponent* SpringArm = ActiveSpringArm.Get())
	{
		SpringArm->TargetOffset = RestTargetOffset;
		SpringArm->TargetArmLength = RestArmLength;
	}

	ElapsedRealTime = 0.f;
	LastRealTimeSeconds = 0.0;
	bFocusActive = false;
	ActiveSpringArm.Reset();
	SetComponentTickEnabled(false);
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

float UCombatCameraDirectorComponent::GetTotalDuration() const
{
	return FocusInDuration + FocusHoldDuration + FocusOutDuration;
}

void UCombatCameraDirectorComponent::ApplyFocusState(float Alpha)
{
	USpringArmComponent* SpringArm = ActiveSpringArm.Get();
	if (!SpringArm)
	{
		return;
	}

	const float InEnd = FocusInDuration;
	const float HoldEnd = FocusInDuration + FocusHoldDuration;
	const float TotalDuration = GetTotalDuration();
	const float CurrentTime = Alpha * TotalDuration;

	FVector NewTargetOffset = FocusTargetOffset;
	float NewArmLength = FocusArmLength;

	if (FocusInDuration > KINDA_SMALL_NUMBER && CurrentTime < InEnd)
	{
		float BlendAlpha = FMath::Clamp(CurrentTime / FocusInDuration, 0.f, 1.f);
		BlendAlpha = BlendAlpha * BlendAlpha * (3.f - 2.f * BlendAlpha);
		NewTargetOffset = FMath::Lerp(StartTargetOffset, FocusTargetOffset, BlendAlpha);
		NewArmLength = FMath::Lerp(StartArmLength, FocusArmLength, BlendAlpha);
	}
	else if (FocusOutDuration > KINDA_SMALL_NUMBER && CurrentTime > HoldEnd)
	{
		float OutAlpha = FMath::Clamp((CurrentTime - HoldEnd) / FocusOutDuration, 0.f, 1.f);
		OutAlpha = OutAlpha * OutAlpha * (3.f - 2.f * OutAlpha);
		NewTargetOffset = FMath::Lerp(FocusTargetOffset, RestTargetOffset, OutAlpha);
		NewArmLength = FMath::Lerp(FocusArmLength, RestArmLength, OutAlpha);
	}

	SpringArm->TargetOffset = NewTargetOffset;
	SpringArm->TargetArmLength = NewArmLength;
}
