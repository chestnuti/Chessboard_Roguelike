#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Grid/GridTypes.h"
#include "TimerManager.h"
#include "Transform/ChessTransformTypes.h"
#include "Tutorial/TutorialFlowData.h"
#include "TutorialFlowComponent.generated.h"

class AGridEnemyManager;
class AGridEnemyPawn;
class AGridManager;
class AGridPawn;
class AGridPickupActor;
class UChessPieceFormData;
class UConversionEnergyComponent;
class UPlayerAttributeComponent;
class UPlayerTransformInventoryComponent;
class UTutorialInstructionWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialFlowCompleted, UTutorialFlowDataAsset*, CompletedFlow);

UCLASS(ClassGroup=(Tutorial), BlueprintType, meta=(BlueprintSpawnableComponent))
class CHESSBOARD_ROGUELIKE_API UTutorialFlowComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTutorialFlowComponent();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void StartTutorialFlow(UTutorialFlowDataAsset* InFlowData);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void StopTutorialFlow();

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void AdvanceManualStep();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Tutorial")
	bool IsTutorialActive() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|UI")
	TSubclassOf<UTutorialInstructionWidget> TutorialWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|UI")
	int32 TutorialWidgetZOrder = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Travel", meta = (ClampMin = "0.0"))
	float NextLevelTravelDelay = 1.0f;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial|Events")
	FOnTutorialFlowCompleted OnTutorialFlowCompleted;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTutorialFlowDataAsset> FlowData;

	UPROPERTY(Transient)
	TObjectPtr<UTutorialInstructionWidget> TutorialWidget;

	UPROPERTY(Transient)
	TObjectPtr<AGridPawn> PlayerPawn;

	UPROPERTY(Transient)
	TObjectPtr<AGridManager> GridManager;

	UPROPERTY(Transient)
	TObjectPtr<AGridEnemyManager> EnemyManager;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerAttributeComponent> AttributeComponent;

	UPROPERTY(Transient)
	TObjectPtr<UConversionEnergyComponent> ConversionEnergyComponent;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerTransformInventoryComponent> TransformInventoryComponent;

	int32 CurrentStepIndex = INDEX_NONE;
	int32 LastObservedHealth = 0;
	int32 CollectedTransformPieceCount = 0;
	bool bActive = false;
	FName PendingNextLevelName = NAME_None;
	FTimerHandle NextLevelTravelTimerHandle;

	void ResolveReferences();
	void BindRuntimeEvents();
	void UnbindRuntimeEvents();
	void BindPickupEvents();
	void BindEnemyEvents();
	void ShowCurrentStep();
	void CompleteCurrentStep();
	void CompleteTutorialFlow();
	void EvaluateCurrentStepImmediately();
	void OpenPendingNextLevel();
	const FTutorialStepDefinition* GetCurrentStep() const;
	bool IsCurrentStepTrigger(ETutorialStepTriggerType TriggerType) const;
	bool AreAllAttributeTilesCleared() const;
	bool IsMatchingTransformType(EChessTransformType CandidateType) const;

	UFUNCTION()
	void HandlePlayerMoved(FIntPoint FromCoord, FIntPoint ToCoord, ETileType EnteredTileType);

	UFUNCTION()
	void HandlePlayerTransformMoveCompleted(UChessPieceFormData* FormData, FIntPoint FromCoord, FIntPoint ToCoord);

	UFUNCTION()
	void HandlePlayerConversionEnergyUsed(ETileType EnergyType, bool bConvertedAny);

	UFUNCTION()
	void HandlePlayerAttributeChanged(int32 NewConstructValue, int32 NewAcidValue);

	UFUNCTION()
	void HandlePlayerHealthChanged(int32 NewHealth, int32 MaxHealth);

	UFUNCTION()
	void HandleConversionEnergyChanged(bool bHasEnergy, ETileType EnergyType);

	UFUNCTION()
	void HandleTileTypeChanged(FIntPoint TileCoord, ETileType NewTileType);

	UFUNCTION()
	void HandleEnemyKilled(AGridEnemyPawn* Enemy, FIntPoint DeathCoord, FVector DeathWorldLocation);

	UFUNCTION()
	void HandleAllEnemiesCleared();

	UFUNCTION()
	void HandlePickupCollected(AGridPickupActor* Pickup, AGridPawn* CollectingPawn, bool bEffectApplied);

	UFUNCTION()
	void HandleTransformInventoryChanged();

	UFUNCTION()
	void HandleTransformWheelOpened();

	UFUNCTION()
	void HandleTransformSelected(UChessPieceFormData* FormData);
};
