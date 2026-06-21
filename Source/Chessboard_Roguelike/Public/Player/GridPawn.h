#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Grid/GridTypes.h"
#include "Transform/ChessTransformTypes.h"
#include "GridPawn.generated.h"

class AGridManager;
class ATurnManager;
class AGridEnemyPawn;
class AGridEnemyManager;
class AGridPickupManager;
class UChessPieceFormData;
class UCombatResolverComponent;
class UConversionEnergyComponent;
class UMaterialInterface;
class UMaterialParameterCollection;
class UStaticMeshComponent;
class USceneComponent;
class UPlayerAttributeComponent;
class UPlayerTransformInventoryComponent;
class UTileEffectResolverComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTransformVisualStateChanged, UChessPieceFormData*, FormData, bool, bIsTransformed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGridPawnMoved, FIntPoint, FromCoord, FIntPoint, ToCoord, ETileType, EnteredTileType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGridPawnEnemyKilled, AGridEnemyPawn*, KilledEnemy, FIntPoint, DeathCoord, ETileType, DroppedEnergyType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGridPawnConversionEnergyUsed, ETileType, EnergyType, bool, bConvertedAny);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGridPawnTransformMoveCompleted, UChessPieceFormData*, FormData, FIntPoint, FromCoord, FIntPoint, ToCoord);

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

	// Resolves melee attacks without owning grid occupancy changes.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCombatResolverComponent> CombatResolverComponent;

	// Owns the player's single-slot tile conversion energy state.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UConversionEnergyComponent> ConversionEnergyComponent;

	// Stores consumable chess-piece forms used by the transform wheel.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPlayerTransformInventoryComponent> TransformInventoryComponent;

	// Authoritative logical position for movement and validation.
	UPROPERTY(BlueprintReadOnly, Category = "Grid")
	FIntPoint CurrentGridCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	FIntPoint StartGridCoord = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	bool bAutoInitializeOnBeginPlay = true;

	// Optional material parameter collection used by grid materials to know the player's current logical tile.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Material")
	TObjectPtr<UMaterialParameterCollection> PlayerGridParameterCollection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Material")
	FName PlayerGridXParameterName = TEXT("PlayerGridX");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid|Material")
	FName PlayerGridYParameterName = TEXT("PlayerGridY");

	// Visual interpolation time only; logical movement resolves immediately after RequestMove succeeds.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MoveDuration = 0.15f;

	// Input lock for the short visual movement window.
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "Transform|Visual")
	bool bIsTransformVisualActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Transform|Visual")
	TObjectPtr<UChessPieceFormData> ActiveTransformVisualForm;

	UPROPERTY(BlueprintAssignable, Category = "Transform|Visual")
	FOnTransformVisualStateChanged OnTransformVisualStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnGridPawnMoved OnGridPawnMoved;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnGridPawnEnemyKilled OnGridPawnEnemyKilled;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnGridPawnConversionEnergyUsed OnGridPawnConversionEnergyUsed;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnGridPawnTransformMoveCompleted OnGridPawnTransformMoveCompleted;

	UPROPERTY()
	TObjectPtr<AGridManager> GridManager;

	UPROPERTY()
	TObjectPtr<ATurnManager> TurnManager;

	UPROPERTY()
	TObjectPtr<AGridEnemyManager> EnemyManager;

	UPROPERTY()
	TObjectPtr<AGridPickupManager> PickupManager;

	UFUNCTION(BlueprintCallable, Category = "Grid")
	void InitializeOnGrid(AGridManager* InGridManager, ATurnManager* InTurnManager, FIntPoint InStartCoord);

	// Direction must be exactly one cardinal grid step.
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void TryMove(FIntPoint Direction);

	UFUNCTION(BlueprintCallable, Category = "Transform")
	bool TryTransformMoveToCoord(FIntPoint TargetCoord, UChessPieceFormData* FormData);

	UFUNCTION(BlueprintCallable, Category = "Transform")
	TArray<FTransformMoveTarget> BuildLegalTransformTargets(UChessPieceFormData* FormData) const;

	UFUNCTION(BlueprintCallable, Category = "Transform|Visual")
	void ApplyTransformVisual(UChessPieceFormData* FormData);

	UFUNCTION(BlueprintCallable, Category = "Transform|Visual")
	void RestoreDefaultPawnVisual();

	UFUNCTION(BlueprintImplementableEvent, Category = "Transform|Visual")
	void OnTransformVisualApplied(UChessPieceFormData* FormData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Transform|Visual")
	void OnTransformVisualRestored(UChessPieceFormData* FormData);

	// Presentation hook kept for Blueprint feedback after C++ grants conversion energy.
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnPlayerKilledEnemy(AGridEnemyPawn* KilledEnemy, ETileType DroppedEnergyType);

	// Convert area to selected type around play
	UFUNCTION(BlueprintCallable, category = "Grid")
	bool ConvertAreaAroundPlayer(AGridManager* InGridManager, ETileType EnergyType);

	UFUNCTION(BlueprintCallable, Category = "Grid|Material")
	void SyncPlayerGridMaterialParameters();

	UFUNCTION(BlueprintCallable, Category = "Grid|Visual")
	void RefreshPlayerNextMoveTiles();

	UFUNCTION(BlueprintCallable, Category = "Combat Preview")
	void RefreshCombatPreview(FIntPoint PreviewPlayerCoord);

	UFUNCTION(BlueprintCallable, Category = "Combat Preview")
	void RefreshDefaultCombatPreview();

	UFUNCTION(BlueprintCallable, Category = "Combat Preview")
	void RefreshTransformCombatPreview(const TArray<FTransformMoveTarget>& TransformTargets, FIntPoint PreviewPlayerCoord);

	UFUNCTION(BlueprintCallable, Category = "Combat Preview")
	void ClearCombatPreview();

	// Visual-only movement; grid occupancy has already been updated before this starts.
	void StartVisualMove(const FVector& From, const FVector& To);
	void FinishVisualMove();

	bool ResolvePickupAtCurrentTile();

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	FVector VisualMoveFrom = FVector::ZeroVector;
	FVector VisualMoveTo = FVector::ZeroVector;
	FVector FailedAttackVisualPeak = FVector::ZeroVector;
	float MoveElapsedTime = 0.f;
	bool bIsFailedAttackVisualMove = false;
	bool bInitializedOnGrid = false;
	bool bRestoreDefaultVisualOnMoveFinish = false;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> DefaultPawnMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DefaultPawnMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGridEnemyPawn>> PreviewedCombatEnemies;

	bool TryResolveMoveOrAttackToCoord(FIntPoint TargetCoord);
	bool ResolveEnemyMeleeAttack(FIntPoint TargetCoord, AGridEnemyPawn* EnemyActor);
	bool IsLegalTransformTarget(FIntPoint TargetCoord, UChessPieceFormData* FormData) const;
	void ApplyCombatPreview(const TSet<FIntPoint>& PlayerAttackCoords, FIntPoint PreviewPlayerCoord);
	void CacheDefaultPawnVisual();
	void StartFailedAttackVisualMove(const FVector& From, const FVector& BlockedTarget);
	void FindEnemyManagerIfNeeded();
	void FindPickupManagerIfNeeded();
	void ResolvePostPlayerActionTurn();
};
