#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCG/DungeonLayoutTypes.h"
#include "DungeonRunManager.generated.h"

class AGridEnemyManager;
class AGridEnemyPawn;
class AGridManager;
class AGridPawn;
class ATurnManager;
class UDungeonGenerationSettings;

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API ADungeonRunManager : public AActor
{
	GENERATED_BODY()

public:
	ADungeonRunManager();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	TObjectPtr<UDungeonGenerationSettings> DungeonGenerationSettings;

	// Main one-shot entry for prototype levels; disable this when a GameMode or UI owns run creation.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	bool bGenerateOnBeginPlay = true;

	// Allows test maps to work with placed managers without explicit Blueprint wiring.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	bool bAutoFindReferences = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	bool bInitializePlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	bool bSpawnEnemies = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	TObjectPtr<AGridManager> GridManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	TObjectPtr<ATurnManager> TurnManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	TObjectPtr<AGridPawn> PlayerPawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	TObjectPtr<AGridEnemyManager> EnemyManager;

	UPROPERTY(BlueprintReadOnly, Category = "PCG|Dungeon")
	FGeneratedDungeonLayout LastGeneratedLayout;

	// Generates a layout, applies it to the grid, then initializes actors that depend on the generated coordinates.
	UFUNCTION(BlueprintCallable, Category = "PCG|Dungeon")
	bool GenerateAndInitializeRun();

	UFUNCTION(BlueprintCallable, Category = "PCG|Dungeon")
	void ResolveRuntimeReferences();

private:
	bool ApplyGeneratedLayout();
	bool InitializePlayerFromLayout();
	void SpawnEnemiesFromLayout();
	TSubclassOf<AGridEnemyPawn> SelectEnemyClassForCandidate(const FDungeonSpawnCandidate& Candidate, FRandomStream& Stream) const;
	void RegisterEnemyWithManager(AGridEnemyPawn* Enemy) const;
};
