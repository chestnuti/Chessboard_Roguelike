#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Grid/GridTypes.h"
#include "ConversionEnergyComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnConversionEnergyChanged,
	bool,
	bHasEnergy,
	ETileType,
	EnergyType);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnConversionEnergyGranted,
	ETileType,
	EnergyType);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnConversionEnergyConsumed,
	ETileType,
	EnergyType);

// Owns the player's single-slot tile conversion energy state.
UCLASS(ClassGroup=(Player), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHESSBOARD_ROGUELIKE_API UConversionEnergyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UConversionEnergyComponent();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Conversion Energy")
	bool HasConversionEnergy() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Conversion Energy")
	ETileType GetHeldConversionEnergyType() const;

	UFUNCTION(BlueprintCallable, Category = "Conversion Energy")
	void GrantConversionEnergy(ETileType NewEnergyType);

	UFUNCTION(BlueprintCallable, Category = "Conversion Energy")
	bool ConsumeConversionEnergy();

	UFUNCTION(BlueprintCallable, Category = "Conversion Energy")
	void ClearConversionEnergy();

	UPROPERTY(BlueprintAssignable, Category = "Conversion Energy|Events")
	FOnConversionEnergyChanged OnConversionEnergyChanged;

	UPROPERTY(BlueprintAssignable, Category = "Conversion Energy|Events")
	FOnConversionEnergyGranted OnConversionEnergyGranted;

	UPROPERTY(BlueprintAssignable, Category = "Conversion Energy|Events")
	FOnConversionEnergyConsumed OnConversionEnergyConsumed;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Conversion Energy", meta = (AllowPrivateAccess = "true"))
	bool bHasConversionEnergy = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Conversion Energy", meta = (AllowPrivateAccess = "true"))
	ETileType HeldConversionEnergyType = ETileType::Minimal;

	bool IsGrantableEnergyType(ETileType EnergyType) const;
};
