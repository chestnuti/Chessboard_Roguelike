#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enemy/EnemyTypes.h"
#include "PlayerAttributeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnPlayerAttributeChanged,
	int32,
	NewConstructValue,
	int32,
	NewAcidValue);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnPlayerHealthChanged,
	int32,
	NewHealth,
	int32,
	MaxHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerDefeated);

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

	UFUNCTION(BlueprintCallable, Category = "Player Attributes|Health")
	bool ApplyHealthDamage(int32 DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Player Attributes|Health")
	bool Heal(int32 HealAmount);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	int32 GetConstructValue() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	int32 GetAcidValue() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes|Health")
	int32 GetCurrentHealth() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes|Health")
	int32 GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	int32 GetMaxConstructValue() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	int32 GetMaxAcidValue() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes|Health")
	float GetHealthRatio() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	float GetConstructRatio() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	float GetAcidRatio() const;

	// Convenience predicates for future ability gates or UI warnings.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	bool IsConstructValueMaxed() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes")
	bool IsAcidValueMaxed() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes|Suppression")
	bool IsConstructSuppressionActive() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes|Suppression")
	bool IsAcidSuppressionActive() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes|Suppression")
	bool CanSuppressFaction(EEnemyFaction EnemyFaction) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player Attributes|Health")
	bool IsDefeated() const;

	UPROPERTY(BlueprintAssignable, Category = "Player Attributes|Health")
	FOnPlayerHealthChanged OnPlayerHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Player Attributes|Health")
	FOnPlayerDefeated OnPlayerDefeated;

	UPROPERTY(BlueprintAssignable, Category = "Player Attributes|Events")
	FOnPlayerAttributeChanged OnPlayerAttributeChanged;

private:
	// Kept private so Blueprint widgets can read values through getters without mutating gameplay state.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Attributes|Health", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 CurrentHealth = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Attributes|Health", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxHealth = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Attributes", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 ConstructValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Attributes", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 AcidValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Attributes", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxConstructValue = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Attributes", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxAcidValue = 10;
};
