#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GridPawn.generated.h"

class AGridManager;
class ATurnManager;
class UStaticMeshComponent;
class USceneComponent;
class UPlayerAttributeComponent;
class UTileEffectResolverComponent;

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGridPawn : public APawn
{
	GENERATED_BODY()

public:
	AGridPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PawnMesh;

	// Gameplay attributes live on a component so tile effects and HUD do not need pawn internals.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPlayerAttributeComponent> PlayerAttributeComponent;

	// Resolves optional tile-enter effects after GridManager confirms movement.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTileEffectResolverComponent> TileEffectResolverComponent;

	// Authoritative logical position for movement and validation.
	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	FIntPoint CurrentGridCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	FIntPoint StartGridCoord = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	bool bAutoInitializeOnBeginPlay = true;

	// Visual interpolation time only; logical movement resolves immediately after RequestMove succeeds.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveDuration = 0.15f;

	// Input lock for the short visual movement window.
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsMoving = false;

	UPROPERTY()
	TObjectPtr<AGridManager> GridManager;

	UPROPERTY()
	TObjectPtr<ATurnManager> TurnManager;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void InitializeOnGrid(AGridManager* InGridManager, ATurnManager* InTurnManager, FIntPoint InStartCoord);

	// Direction must be exactly one cardinal grid step.
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void TryMove(FIntPoint Direction);

	// Visual-only movement; grid occupancy has already been updated before this starts.
	void StartVisualMove(const FVector& From, const FVector& To);
	void FinishVisualMove();

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	FVector VisualMoveFrom = FVector::ZeroVector;
	FVector VisualMoveTo = FVector::ZeroVector;
	float MoveElapsedTime = 0.f;
	bool bInitializedOnGrid = false;
};
