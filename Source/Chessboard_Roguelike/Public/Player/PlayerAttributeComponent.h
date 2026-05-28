#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerAttributeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnPlayerAttributeChanged,
	int32,
	NewConstructValue,
	int32,
	NewAcidValue);

// Owns the player's Construct/Acid values; UI reads or listens, but does not write them.
UCLASS(ClassGroup=(Player), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHESSBOARD_ROGUELIKE_API UPlayerAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerAttributeComponent();

	// Applies both values together so UI receives one combined change event.
	UFUNCTION(BlueprintCallable, Category = "Player Attributes")
	void ApplyTileAttributeDelta(int32 ConstructDelta, int32 AcidDelta);

	UFUNCTION(BlueprintCallable, Category = "Player Attributes")
	void AddConstructValue(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "Player Attributes")
	void AddAcidValue(int32 Delta);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	int32 GetConstructValue() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	int32 GetAcidValue() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	int32 GetMaxConstructValue() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	int32 GetMaxAcidValue() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	float GetConstructRatio() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	float GetAcidRatio() const;

	// Convenience predicates for future ability gates or UI warnings.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	bool IsConstructValueMaxed() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	bool IsAcidValueMaxed() const;

	UPROPERTY(BlueprintAssignable, Category = "Player Attributes|Events")
	FOnPlayerAttributeChanged OnPlayerAttributeChanged;

private:
	// Kept private so Blueprint widgets can read values through getters without mutating gameplay state.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Attributes", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 ConstructValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Attributes", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 AcidValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Attributes", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxConstructValue = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Attributes", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxAcidValue = 10;
};
