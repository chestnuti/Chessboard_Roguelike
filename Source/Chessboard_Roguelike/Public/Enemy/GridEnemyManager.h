#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridEnemyManager.generated.h"

class AGridEnemyPawn;
class AGridManager;
class AGridPawn;
class ATurnManager;

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGridEnemyManager : public AActor
{
	GENERATED_BODY()

public:
	AGridEnemyManager();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void InitializeEnemyManager(AGridManager* InGridManager, ATurnManager* InTurnManager, AGridPawn* InPlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void RegisterEnemy(AGridEnemyPawn* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void UnregisterEnemy(AGridEnemyPawn* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void RebuildEnemyList();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void ExecuteEnemyTurn();

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

private:
	void AutoInitializeReferences();
	void PruneInvalidEnemies();
};
