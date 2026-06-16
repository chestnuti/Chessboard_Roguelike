#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridEnemyManager.generated.h"

class AGridEnemyPawn;
class AGridManager;
class AGridPawn;
class ATurnManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllEnemiesCleared);

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGridEnemyManager : public AActor
{
	GENERATED_BODY()

public:
	AGridEnemyManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void InitializeEnemyManager(AGridManager* InGridManager, ATurnManager* InTurnManager, AGridPawn* InPlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void RegisterEnemy(AGridEnemyPawn* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void UnregisterEnemy(AGridEnemyPawn* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void RebuildEnemyList();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ClearAllEnemies();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ExecuteEnemyTurn();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Enemy")
	bool ShouldDelayEnemyTurnEnd() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Enemy")
	TArray<AGridEnemyPawn*> GetAliveEnemies() const;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy")
	TArray<TObjectPtr<AGridEnemyPawn>> RegisteredEnemies;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	TObjectPtr<AGridManager> GridManager;

	UPROPERTY(BlueprintReadOnly, Category = "Turn")
	TObjectPtr<ATurnManager> TurnManager;

	UPROPERTY(BlueprintReadOnly, Category = "Player")
	TObjectPtr<AGridPawn> PlayerPawn;

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnAllEnemiesCleared OnAllEnemiesCleared;

private:
	UFUNCTION()
	void HandleEnemyKilled(AGridEnemyPawn* Enemy, FIntPoint DeathCoord, FVector DeathWorldLocation);

	void AutoInitializeReferences();
	void PruneInvalidEnemies();
	bool HasMovingEnemies() const;
	int32 ResolvePendingRangedAttacks(TSet<AGridEnemyPawn*>& OutResolvedAttackers);

	bool bWaitingForEnemyMovement = false;
};
