#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatCameraDirectorComponent.generated.h"

class USpringArmComponent;

UCLASS(ClassGroup=(Camera), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHESSBOARD_ROGUELIKE_API UCombatCameraDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatCameraDirectorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Camera", meta = (ClampMin = "0.0"))
	float FocusInDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Camera", meta = (ClampMin = "0.0"))
	float FocusHoldDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Camera", meta = (ClampMin = "0.0"))
	float FocusOutDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Camera", meta = (ClampMin = "0.0"))
	float ExtraFocusInDurationAtMaxDistance = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Camera", meta = (ClampMin = "0.0"))
	float ExtraFocusHoldDurationAtMaxDistance = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Camera", meta = (ClampMin = "0.0"))
	float MaxFocusOffset = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Camera", meta = (ClampMin = "0.0"))
	float FocusOffsetScale = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Camera", meta = (ClampMin = "0.0"))
	float ZoomInDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Camera")
	bool bDisableSpringArmLagDuringFocus = true;

	UFUNCTION(BlueprintCallable, Category = "Combat Camera")
	void FocusGridTileBriefly(const FVector& TargetWorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Combat Camera")
	void StopFocus();

private:
	TWeakObjectPtr<USpringArmComponent> ActiveSpringArm;

	FVector RestTargetOffset = FVector::ZeroVector;
	FVector StartTargetOffset = FVector::ZeroVector;
	FVector FocusTargetOffset = FVector::ZeroVector;

	float RestArmLength = 0.f;
	float StartArmLength = 0.f;
	float FocusArmLength = 0.f;
	float RuntimeFocusInDuration = 0.f;
	float RuntimeFocusHoldDuration = 0.f;
	float RuntimeFocusOutDuration = 0.f;
	float ElapsedRealTime = 0.f;
	double LastRealTimeSeconds = 0.0;
	float RestCameraLagSpeed = 0.f;
	float RestCameraRotationLagSpeed = 0.f;
	bool bFocusActive = false;
	bool bRestCameraLagEnabled = false;
	bool bRestCameraRotationLagEnabled = false;
	bool bHasCachedSpringArmLagSettings = false;

	USpringArmComponent* FindSpringArm();
	FVector CalculateFocusTargetOffset(const USpringArmComponent* SpringArm, const FVector& TargetWorldLocation) const;
	float CalculateFocusDistanceAlpha(const USpringArmComponent* SpringArm, const FVector& TargetWorldLocation) const;
	float GetTotalDuration() const;
	void ApplyFocusState(float Alpha);
	void CacheAndDisableSpringArmLag(USpringArmComponent* SpringArm);
	void RestoreSpringArmLag(USpringArmComponent* SpringArm);
};
