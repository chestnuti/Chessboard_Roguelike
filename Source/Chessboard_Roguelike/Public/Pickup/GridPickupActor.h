#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grid/GridTypes.h"
#include "GridPickupActor.generated.h"

class AGridManager;
class AGridPawn;
class AGridPickupActor;
class UStaticMeshComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnGridPickupCollected,
	AGridPickupActor*,
	Pickup,
	AGridPawn*,
	PlayerPawn,
	bool,
	bEffectApplied);

// Base class for grid-positioned pickups. Pickups are tracked separately from GridManager occupancy.
UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGridPickupActor : public AActor
{
	GENERATED_BODY()

public:
	AGridPickupActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(BlueprintReadOnly, Category = "Pickup|Grid")
	FIntPoint GridCoord = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Pickup|Grid")
	TObjectPtr<AGridManager> GridManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	FName PickupId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	bool bConsumeOnSuccessfulEffect = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	bool bConsumeWhenEffectFails = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup|Visual")
	float VisualZOffset = 50.f;

	UPROPERTY(BlueprintAssignable, Category = "Pickup|Events")
	FOnGridPickupCollected OnGridPickupCollected;

	UFUNCTION(BlueprintCallable, Category = "Pickup|Grid")
	void InitializeOnGrid(AGridManager* InGridManager, FIntPoint InGridCoord);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pickup|Grid")
	FIntPoint GetGridCoord() const;

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	bool TryCollect(AGridPawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "Pickup")
	bool CanCollect(AGridPawn* PlayerPawn) const;
	virtual bool CanCollect_Implementation(AGridPawn* PlayerPawn) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Pickup")
	bool ApplyPickupEffect(AGridPawn* PlayerPawn);
	virtual bool ApplyPickupEffect_Implementation(AGridPawn* PlayerPawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup|Events")
	void OnCollected(AGridPawn* PlayerPawn, bool bEffectApplied);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(BlueprintReadOnly, Category = "Pickup")
	bool bCollected = false;
};
