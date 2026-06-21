#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatTypes.h"
#include "Enemy/EnemyTypes.h"
#include "GameFramework/Pawn.h"
#include "GridEnemyPawn.generated.h"

class AGridManager;
class AGridPawn;
class AGridEnemyPawn;
class URangedAttackTelegraphComponent;
class USceneComponent;
class UStaticMeshComponent;
class UEnemyAudioProfileDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGridEnemyKilled, AGridEnemyPawn*, Enemy, FIntPoint, DeathCoord, FVector, DeathWorldLocation);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<URangedAttackTelegraphComponent> RangedAttackTelegraphComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EEnemyFaction Faction = EEnemyFaction::Construct;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EEnemyBehaviorType BehaviorType = EEnemyBehaviorType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Audio")
	TObjectPtr<UEnemyAudioProfileDataAsset> AudioProfile;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy")
	EEnemyActionState ActionState = EEnemyActionState::Idle;

	// This is a single-hit kill threshold, not a traditional damage pool.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "1"))
	int32 KillThreshold = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Attack", meta = (ClampMin = "0"))
	int32 AttackDamage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Attack", meta = (ClampMin = "0"))
	int32 AttackAttributeDamage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Attack")
	bool bApplyFactionAttributeDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Ranged", meta = (ClampMin = "0"))
	int32 MaxRangedAttackDistance = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Ranged")
	TArray<FIntPoint> PendingRangedAttackTiles;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Ranged")
	FIntPoint PendingRangedAttackDirection = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	FIntPoint CurrentGridCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	FIntPoint StartGridCoord = FIntPoint(1, 0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	bool bAutoInitializeOnBeginPlay = true;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy")
	bool bDead = false;

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FOnGridEnemyKilled OnGridEnemyKilled;

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

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Enemy|Preview")
	bool CanAttackCoordNextTurn(FIntPoint TargetCoord, const AGridPawn* PlayerPawn) const;

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool TryMoveToGridCoord(FIntPoint TargetCoord);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Enemy")
	bool ExecuteBasicTurn(AGridPawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Enemy|Ranged")
	bool HasPendingRangedAttack() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Ranged")
	bool ResolvePendingRangedAttack(AGridPawn* PlayerPawn);

	void PlayRangedAttackAudio();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Ranged")
	void ClearRangedAimMode();

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Suppression")
	void OnSuppressedByPlayer(AGridPawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Enemy")
	void ExecuteMeleeAttack(AGridPawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Attack")
	FEnemyAttackResolveResult ApplyMeleeAttackDamage(AGridPawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Attack")
	FEnemyAttackResolveResult ApplyRangedAttackDamage(AGridPawn* PlayerPawn);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Friendly Fire")
	bool ApplyRangedFriendlyFireDamage(AGridEnemyPawn* TargetEnemy);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Attack")
	void OnMeleeAttackResolved(AGridPawn* PlayerPawn, FEnemyAttackResolveResult AttackResult);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Friendly Fire")
	void OnFriendlyFireResolved(AGridEnemyPawn* OtherEnemy, FEnemyFriendlyFireResolveResult ResolveResult);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Ranged")
	void OnRangedAimStarted(AGridPawn* PlayerPawn, const TArray<FIntPoint>& AttackTiles);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Ranged")
	void OnRangedAttackResolved(AGridPawn* PlayerPawn, const TArray<FIntPoint>& AttackTiles, FEnemyAttackResolveResult AttackResult, bool bHitPlayer);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Ranged")
	void OnRangedAimCleared();

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

	bool EnterRangedAimMode(AGridPawn* PlayerPawn);
	bool TryMoveTowardRangedAlignment(AGridPawn* PlayerPawn);
	bool HasLineOfSightToPlayer(const AGridPawn* PlayerPawn, TArray<FIntPoint>& OutLineTiles, FIntPoint& OutDirection) const;
	bool BuildRangedLineFromCoord(FIntPoint StartCoord, FIntPoint Direction, TArray<FIntPoint>& OutLineTiles) const;
	static bool TryGetAxisDirection(FIntPoint FromCoord, FIntPoint ToCoord, FIntPoint& OutDirection);

	void StartAttackReboundVisualMove(const FVector& From, const FVector& BlockedTarget);
	void ResolveDefeatedPlayerOccupancy(AGridPawn* PlayerPawn);
};
