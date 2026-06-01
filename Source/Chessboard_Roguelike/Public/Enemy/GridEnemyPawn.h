#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"
#include "Enemy/EnemyTypes.h"
#include "GameFramework/Pawn.h"
#include "GridEnemyPawn.generated.h"

class AGridManager;
class AGridPawn;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGridEnemyPawn : public APawn
{
	GENERATED_BODY()

public:
	AGridEnemyPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> EnemyMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EEnemyFaction Faction = EEnemyFaction::Construct;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EEnemyBehaviorType BehaviorType = EEnemyBehaviorType::Melee;

	// This is a single-hit kill threshold, not a traditional damage pool.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "1"))
	int32 KillThreshold = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Attack", meta = (ClampMin = "0"))
	int32 AttackDamage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Attack", meta = (ClampMin = "0"))
	int32 AttackAttributeDamage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Attack")
	bool bApplyFactionAttributeDamage = true;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	FIntPoint CurrentGridCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	FIntPoint StartGridCoord = FIntPoint(1, 0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	bool bAutoInitializeOnBeginPlay = true;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy")
	bool bDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveDuration = 0.15f;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsMoving = false;

	UPROPERTY()
	TObjectPtr<AGridManager> GridManager;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void InitializeOnGrid(AGridManager* InGridManager, FIntPoint InStartCoord);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Enemy")
	bool CanReceiveDamage() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Enemy")
	bool IsAlive() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Enemy")
	bool CanAct() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Enemy|Suppression")
	bool IsSuppressedByPlayer(const AGridPawn* PlayerPawn) const;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool TryMoveToGridCoord(FIntPoint TargetCoord);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Enemy")
	bool ExecuteBasicTurn(AGridPawn* PlayerPawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Suppression")
	void OnSuppressedByPlayer(AGridPawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Enemy")
	void ExecuteMeleeAttack(AGridPawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Attack")
	FEnemyAttackResolveResult ApplyMeleeAttackDamage(AGridPawn* PlayerPawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Attack")
	void OnMeleeAttackResolved(AGridPawn* PlayerPawn, FEnemyAttackResolveResult AttackResult);

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void Kill();

	void StartVisualMove(const FVector& From, const FVector& To);
	void FinishVisualMove();

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	FVector VisualMoveFrom = FVector::ZeroVector;
	FVector VisualMoveTo = FVector::ZeroVector;
	FVector AttackReboundVisualPeak = FVector::ZeroVector;
	float MoveElapsedTime = 0.f;
	bool bIsAttackReboundVisualMove = false;
	bool bInitializedOnGrid = false;
	bool bMeleeDamageResolvedInCurrentAttack = false;
	FEnemyAttackResolveResult LastMeleeAttackResult;

	void StartAttackReboundVisualMove(const FVector& From, const FVector& BlockedTarget);
	void ResolveDefeatedPlayerOccupancy(AGridPawn* PlayerPawn);
};
