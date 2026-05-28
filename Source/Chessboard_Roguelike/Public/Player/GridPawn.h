#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GridPawn.generated.h"

class AGridManager;
class ATurnManager;
class UStaticMeshComponent;
class USceneComponent;

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

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	FIntPoint CurrentGridCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	FIntPoint StartGridCoord = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	bool bAutoInitializeOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveDuration = 0.15f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsMoving = false;

	UPROPERTY()
	TObjectPtr<AGridManager> GridManager;

	UPROPERTY()
	TObjectPtr<ATurnManager> TurnManager;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void InitializeOnGrid(AGridManager* InGridManager, ATurnManager* InTurnManager, FIntPoint InStartCoord);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void TryMove(FIntPoint Direction);

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
