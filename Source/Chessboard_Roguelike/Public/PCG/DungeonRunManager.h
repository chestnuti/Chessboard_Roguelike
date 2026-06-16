#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCG/DungeonLayoutTypes.h"
#include "DungeonRunManager.generated.h"

class AGridEnemyManager;
class AGridEnemyPawn;
class AGridManager;
class AGridPawn;
class AGridPickupManager;
class AGridPickupActor;
class ATurnManager;
class UDungeonGenerationSettings;
class UTutorialLevelSet;
struct FDungeonEnemySpawnEntry;
struct FDungeonPickupSpawnEntry;
struct FTutorialLevelDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDungeonLevelStarted, int32, LevelIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDungeonLevelCompleted, int32, CompletedLevelIndex, int32, NextLevelIndex);

UENUM(BlueprintType)
enum class EDungeonRunGenerationMode : uint8
{
	Procedural UMETA(DisplayName = "Procedural"),
	TutorialFixed UMETA(DisplayName = "Tutorial Fixed")
};

UENUM(BlueprintType)
enum class EDungeonRunSeedMode : uint8
{
	ConfiguredSeed UMETA(DisplayName = "Configured Seed"),
	RandomSeedPerLevelStart UMETA(DisplayName = "Random Seed Per Level Start")
};

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API ADungeonRunManager : public AActor
{
	GENERATED_BODY()

public:
	ADungeonRunManager();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	TObjectPtr<UDungeonGenerationSettings> DungeonGenerationSettings;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "PCG|Dungeon")
	TObjectPtr<UDungeonGenerationSettings> RuntimeDungeonGenerationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	EDungeonRunGenerationMode GenerationMode = EDungeonRunGenerationMode::Procedural;

	// Optional. When unset, TutorialFixed mode uses the built-in tutorial definitions.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Dungeon")
	TObjectPtr<UTutorialLevelSet> TutorialLevelSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Dungeon", meta = (ClampMin = "0"))
	int32 TutorialLevelIndex = 0;

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
	bool bSpawnPickups = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon|Progression", meta = (ClampMin = "1"))
	int32 CurrentDungeonLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon|Progression")
	bool bUseLevelScaling = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon|Seed")
	EDungeonRunSeedMode SeedMode = EDungeonRunSeedMode::RandomSeedPerLevelStart;

	UPROPERTY(BlueprintReadOnly, Category = "Dungeon|Seed")
	int32 CurrentRunSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon|Progression")
	bool bAutoAdvanceToNextLevel = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon|Progression", meta = (ClampMin = "0.0"))
	float AutoAdvanceDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	TObjectPtr<AGridManager> GridManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	TObjectPtr<ATurnManager> TurnManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	TObjectPtr<AGridPawn> PlayerPawn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	TObjectPtr<AGridEnemyManager> EnemyManager;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PCG|Dungeon")
	TObjectPtr<AGridPickupManager> PickupManager;

	UPROPERTY(BlueprintReadOnly, Category = "PCG|Dungeon")
	FGeneratedDungeonLayout LastGeneratedLayout;

	UPROPERTY(BlueprintAssignable, Category = "Dungeon|Progression")
	FOnDungeonLevelStarted OnDungeonLevelStarted;

	UPROPERTY(BlueprintAssignable, Category = "Dungeon|Progression")
	FOnDungeonLevelCompleted OnDungeonLevelCompleted;

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Progression")
	bool StartLevel(int32 LevelIndex);

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Progression")
	bool AdvanceToNextLevel();

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Progression")
	void CompleteCurrentLevel();

	// Generates a layout, applies it to the grid, then initializes actors that depend on the generated coordinates.
	UFUNCTION(BlueprintCallable, Category = "PCG|Dungeon")
	bool GenerateAndInitializeRun();

	UFUNCTION(BlueprintCallable, Category = "PCG|Dungeon")
	void ResolveRuntimeReferences();

private:
	bool GenerateTutorialRun();
	bool ApplyTutorialLevel(const FTutorialLevelDefinition& TutorialLevel);
	bool InitializePlayerAtCoord(FIntPoint StartCoord);
	void SpawnTutorialEnemies(const FTutorialLevelDefinition& TutorialLevel);
	void SpawnTutorialPickups(const FTutorialLevelDefinition& TutorialLevel);
	bool ApplyGeneratedLayout();
	bool InitializePlayerFromLayout();
	void SpawnEnemiesFromLayout();
	void SpawnPickupsFromLayout();
	void ClearRuntimeActorsForLevelTransition();
	void BindEnemyClearEvent();
	void AdvanceToNextLevelFromTimer();
	UDungeonGenerationSettings* BuildRuntimeSettingsForLevel(int32 LevelIndex);
	const UDungeonGenerationSettings* GetActiveDungeonGenerationSettings() const;
	int32 ResolveSeedForLevel(int32 LevelIndex) const;
	int32 GenerateRandomDungeonSeed() const;
	const FDungeonEnemySpawnEntry* SelectEnemySpawnEntryForCandidate(const FDungeonSpawnCandidate& Candidate, FRandomStream& Stream) const;
	const FDungeonPickupSpawnEntry* SelectPickupSpawnEntryForCandidate(const FDungeonSpawnCandidate& Candidate, FRandomStream& Stream) const;
	int32 CalculateEnemyKillThresholdForCandidate(
		const AGridEnemyPawn* Enemy,
		const FDungeonSpawnCandidate& Candidate,
		const FDungeonEnemySpawnEntry& SpawnEntry) const;
	void RegisterEnemyWithManager(AGridEnemyPawn* Enemy) const;
	AGridPickupManager* EnsurePickupManager();

	bool bCurrentLevelCompleted = false;
};
