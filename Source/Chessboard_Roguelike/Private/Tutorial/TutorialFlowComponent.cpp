#include "Tutorial/TutorialFlowComponent.h"

#include "Blueprint/UserWidget.h"
#include "Data/ChessPieceFormData.h"
#include "Enemy/GridEnemyManager.h"
#include "Enemy/GridEnemyPawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Grid/GridManager.h"
#include "Kismet/GameplayStatics.h"
#include "Pickup/GridPickupActor.h"
#include "Pickup/GridTransformPiecePickupActor.h"
#include "Player/ConversionEnergyComponent.h"
#include "Player/GridPawn.h"
#include "Player/GridPlayerController.h"
#include "Player/PlayerAttributeComponent.h"
#include "Player/PlayerTransformInventoryComponent.h"
#include "UI/TutorialInstructionWidget.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogTutorialFlow, Log, All);

UTutorialFlowComponent::UTutorialFlowComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	TutorialWidgetClass = UTutorialInstructionWidget::StaticClass();

	static ConstructorHelpers::FClassFinder<UTutorialInstructionWidget> TutorialInstructionWidgetClass(
		TEXT("/Game/UI/WBP_TutorialInstruction"));
	if (TutorialInstructionWidgetClass.Succeeded())
	{
		TutorialWidgetClass = TutorialInstructionWidgetClass.Class;
	}
}

void UTutorialFlowComponent::StartTutorialFlow(UTutorialFlowDataAsset* InFlowData)
{
	StopTutorialFlow();

	if (!InFlowData || InFlowData->Steps.IsEmpty())
	{
		UE_LOG(LogTutorialFlow, Warning, TEXT("StartTutorialFlow skipped: FlowData=%s StepCount=%d."),
			*GetNameSafe(InFlowData),
			InFlowData ? InFlowData->Steps.Num() : 0);
		return;
	}

	FlowData = InFlowData;
	CurrentStepIndex = 0;
	bActive = true;
	CollectedTransformPieceCount = 0;

	ResolveReferences();
	UE_LOG(LogTutorialFlow, Log, TEXT("StartTutorialFlow: Flow=%s StepCount=%d Owner=%s PlayerPawn=%s GridManager=%s EnemyManager=%s WidgetClass=%s."),
		*GetNameSafe(FlowData),
		FlowData->Steps.Num(),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(PlayerPawn),
		*GetNameSafe(GridManager),
		*GetNameSafe(EnemyManager),
		*GetNameSafe(TutorialWidgetClass.Get()));
	BindRuntimeEvents();
	ShowCurrentStep();
}

void UTutorialFlowComponent::StopTutorialFlow()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NextLevelTravelTimerHandle);
	}
	PendingNextLevelName = NAME_None;

	UnbindRuntimeEvents();

	if (TutorialWidget)
	{
		TutorialWidget->RemoveFromParent();
		TutorialWidget = nullptr;
	}

	FlowData = nullptr;
	CurrentStepIndex = INDEX_NONE;
	bActive = false;
	CollectedTransformPieceCount = 0;
}

void UTutorialFlowComponent::AdvanceManualStep()
{
	if (IsCurrentStepTrigger(ETutorialStepTriggerType::Manual))
	{
		CompleteCurrentStep();
	}
}

bool UTutorialFlowComponent::IsTutorialActive() const
{
	return bActive;
}

void UTutorialFlowComponent::ResolveReferences()
{
	AActor* Owner = GetOwner();
	AGridPlayerController* PlayerController = Cast<AGridPlayerController>(Owner);
	PlayerPawn = PlayerController ? Cast<AGridPawn>(PlayerController->GetPawn()) : nullptr;

	if (!PlayerPawn && GetWorld())
	{
		for (TActorIterator<AGridPawn> It(GetWorld()); It; ++It)
		{
			PlayerPawn = *It;
			break;
		}
	}

	GridManager = PlayerPawn ? PlayerPawn->GridManager.Get() : nullptr;
	EnemyManager = PlayerPawn ? PlayerPawn->EnemyManager.Get() : nullptr;
	AttributeComponent = PlayerPawn ? PlayerPawn->PlayerAttributeComponent.Get() : nullptr;
	ConversionEnergyComponent = PlayerPawn ? PlayerPawn->ConversionEnergyComponent.Get() : nullptr;
	TransformInventoryComponent = PlayerPawn ? PlayerPawn->TransformInventoryComponent.Get() : nullptr;
	LastObservedHealth = AttributeComponent ? AttributeComponent->GetCurrentHealth() : 0;

	if (!GridManager && GetWorld())
	{
		for (TActorIterator<AGridManager> It(GetWorld()); It; ++It)
		{
			GridManager = *It;
			break;
		}
	}

	if (!EnemyManager && GetWorld())
	{
		for (TActorIterator<AGridEnemyManager> It(GetWorld()); It; ++It)
		{
			EnemyManager = *It;
			break;
		}
	}
}

void UTutorialFlowComponent::BindRuntimeEvents()
{
	if (PlayerPawn)
	{
		PlayerPawn->OnGridPawnMoved.AddDynamic(this, &UTutorialFlowComponent::HandlePlayerMoved);
		PlayerPawn->OnGridPawnTransformMoveCompleted.AddDynamic(this, &UTutorialFlowComponent::HandlePlayerTransformMoveCompleted);
		PlayerPawn->OnGridPawnConversionEnergyUsed.AddDynamic(this, &UTutorialFlowComponent::HandlePlayerConversionEnergyUsed);
	}

	if (AttributeComponent)
	{
		AttributeComponent->OnPlayerAttributeChanged.AddDynamic(this, &UTutorialFlowComponent::HandlePlayerAttributeChanged);
		AttributeComponent->OnPlayerHealthChanged.AddDynamic(this, &UTutorialFlowComponent::HandlePlayerHealthChanged);
	}

	if (ConversionEnergyComponent)
	{
		ConversionEnergyComponent->OnConversionEnergyChanged.AddDynamic(this, &UTutorialFlowComponent::HandleConversionEnergyChanged);
	}

	if (TransformInventoryComponent)
	{
		TransformInventoryComponent->OnTransformInventoryChanged.AddDynamic(this, &UTutorialFlowComponent::HandleTransformInventoryChanged);
	}

	if (GridManager)
	{
		GridManager->OnTileTypeChanged.AddDynamic(this, &UTutorialFlowComponent::HandleTileTypeChanged);
	}

	if (EnemyManager)
	{
		EnemyManager->OnAllEnemiesCleared.AddDynamic(this, &UTutorialFlowComponent::HandleAllEnemiesCleared);
	}

	if (AGridPlayerController* PlayerController = Cast<AGridPlayerController>(GetOwner()))
	{
		PlayerController->OnTransformWheelOpened.AddDynamic(this, &UTutorialFlowComponent::HandleTransformWheelOpened);
		PlayerController->OnTransformSelectedForTutorial.AddDynamic(this, &UTutorialFlowComponent::HandleTransformSelected);
	}

	BindEnemyEvents();
	BindPickupEvents();
}

void UTutorialFlowComponent::UnbindRuntimeEvents()
{
	if (PlayerPawn)
	{
		PlayerPawn->OnGridPawnMoved.RemoveDynamic(this, &UTutorialFlowComponent::HandlePlayerMoved);
		PlayerPawn->OnGridPawnTransformMoveCompleted.RemoveDynamic(this, &UTutorialFlowComponent::HandlePlayerTransformMoveCompleted);
		PlayerPawn->OnGridPawnConversionEnergyUsed.RemoveDynamic(this, &UTutorialFlowComponent::HandlePlayerConversionEnergyUsed);
	}

	if (AttributeComponent)
	{
		AttributeComponent->OnPlayerAttributeChanged.RemoveDynamic(this, &UTutorialFlowComponent::HandlePlayerAttributeChanged);
		AttributeComponent->OnPlayerHealthChanged.RemoveDynamic(this, &UTutorialFlowComponent::HandlePlayerHealthChanged);
	}

	if (ConversionEnergyComponent)
	{
		ConversionEnergyComponent->OnConversionEnergyChanged.RemoveDynamic(this, &UTutorialFlowComponent::HandleConversionEnergyChanged);
	}

	if (TransformInventoryComponent)
	{
		TransformInventoryComponent->OnTransformInventoryChanged.RemoveDynamic(this, &UTutorialFlowComponent::HandleTransformInventoryChanged);
	}

	if (GridManager)
	{
		GridManager->OnTileTypeChanged.RemoveDynamic(this, &UTutorialFlowComponent::HandleTileTypeChanged);
	}

	if (EnemyManager)
	{
		EnemyManager->OnAllEnemiesCleared.RemoveDynamic(this, &UTutorialFlowComponent::HandleAllEnemiesCleared);
	}

	if (AGridPlayerController* PlayerController = Cast<AGridPlayerController>(GetOwner()))
	{
		PlayerController->OnTransformWheelOpened.RemoveDynamic(this, &UTutorialFlowComponent::HandleTransformWheelOpened);
		PlayerController->OnTransformSelectedForTutorial.RemoveDynamic(this, &UTutorialFlowComponent::HandleTransformSelected);
	}

	if (GetWorld())
	{
		for (TActorIterator<AGridEnemyPawn> It(GetWorld()); It; ++It)
		{
			It->OnGridEnemyKilled.RemoveDynamic(this, &UTutorialFlowComponent::HandleEnemyKilled);
		}

		for (TActorIterator<AGridPickupActor> It(GetWorld()); It; ++It)
		{
			It->OnGridPickupCollected.RemoveDynamic(this, &UTutorialFlowComponent::HandlePickupCollected);
		}
	}

	PlayerPawn = nullptr;
	GridManager = nullptr;
	EnemyManager = nullptr;
	AttributeComponent = nullptr;
	ConversionEnergyComponent = nullptr;
	TransformInventoryComponent = nullptr;
}

void UTutorialFlowComponent::BindPickupEvents()
{
	if (!GetWorld())
	{
		return;
	}

	for (TActorIterator<AGridPickupActor> It(GetWorld()); It; ++It)
	{
		It->OnGridPickupCollected.RemoveDynamic(this, &UTutorialFlowComponent::HandlePickupCollected);
		It->OnGridPickupCollected.AddDynamic(this, &UTutorialFlowComponent::HandlePickupCollected);
	}
}

void UTutorialFlowComponent::BindEnemyEvents()
{
	if (!GetWorld())
	{
		return;
	}

	for (TActorIterator<AGridEnemyPawn> It(GetWorld()); It; ++It)
	{
		It->OnGridEnemyKilled.RemoveDynamic(this, &UTutorialFlowComponent::HandleEnemyKilled);
		It->OnGridEnemyKilled.AddDynamic(this, &UTutorialFlowComponent::HandleEnemyKilled);
	}
}

void UTutorialFlowComponent::ShowCurrentStep()
{
	const FTutorialStepDefinition* Step = GetCurrentStep();
	AGridPlayerController* PlayerController = Cast<AGridPlayerController>(GetOwner());
	if (!Step || !PlayerController || !TutorialWidgetClass)
	{
		UE_LOG(LogTutorialFlow, Warning, TEXT("ShowCurrentStep skipped: Step=%s PlayerController=%s WidgetClass=%s CurrentStepIndex=%d Flow=%s."),
			Step ? TEXT("Valid") : TEXT("Null"),
			*GetNameSafe(PlayerController),
			*GetNameSafe(TutorialWidgetClass.Get()),
			CurrentStepIndex,
			*GetNameSafe(FlowData));
		return;
	}

	if (!TutorialWidget)
	{
		TutorialWidget = CreateWidget<UTutorialInstructionWidget>(PlayerController, TutorialWidgetClass);
		UE_LOG(LogTutorialFlow, Log, TEXT("ShowCurrentStep: CreateWidget result=%s Class=%s OwningController=%s."),
			*GetNameSafe(TutorialWidget),
			*GetNameSafe(TutorialWidgetClass.Get()),
			*GetNameSafe(PlayerController));
	}

	if (TutorialWidget)
	{
		TutorialWidget->SetInstruction(Step->InstructionText, CurrentStepIndex + 1, FlowData ? FlowData->Steps.Num() : 0);
		if (!TutorialWidget->IsInViewport())
		{
			TutorialWidget->AddToViewport(TutorialWidgetZOrder);
			UE_LOG(LogTutorialFlow, Log, TEXT("ShowCurrentStep: Added widget to viewport. Flow=%s Step=%d/%d Widget=%s ZOrder=%d."),
				*GetNameSafe(FlowData),
				CurrentStepIndex + 1,
				FlowData ? FlowData->Steps.Num() : 0,
				*GetNameSafe(TutorialWidget),
				TutorialWidgetZOrder);
		}
		else
		{
			UE_LOG(LogTutorialFlow, Verbose, TEXT("ShowCurrentStep: Widget already in viewport. Flow=%s Step=%d/%d."),
				*GetNameSafe(FlowData),
				CurrentStepIndex + 1,
				FlowData ? FlowData->Steps.Num() : 0);
		}
	}
	else
	{
		UE_LOG(LogTutorialFlow, Warning, TEXT("ShowCurrentStep failed: CreateWidget returned null for Class=%s."),
			*GetNameSafe(TutorialWidgetClass.Get()));
	}

	EvaluateCurrentStepImmediately();
}

void UTutorialFlowComponent::CompleteCurrentStep()
{
	if (!FlowData)
	{
		StopTutorialFlow();
		return;
	}

	++CurrentStepIndex;
	if (!FlowData->Steps.IsValidIndex(CurrentStepIndex))
	{
		CompleteTutorialFlow();
		return;
	}

	ShowCurrentStep();
}

void UTutorialFlowComponent::CompleteTutorialFlow()
{
	UTutorialFlowDataAsset* CompletedFlow = FlowData;
	const FName NextLevelName = CompletedFlow ? CompletedFlow->NextLevelName : NAME_None;

	UE_LOG(LogTutorialFlow, Log, TEXT("CompleteTutorialFlow: Flow=%s NextLevelName=%s Delay=%.2f."),
		*GetNameSafe(CompletedFlow),
		*NextLevelName.ToString(),
		NextLevelTravelDelay);

	OnTutorialFlowCompleted.Broadcast(CompletedFlow);
	StopTutorialFlow();

	if (NextLevelName.IsNone())
	{
		return;
	}

	PendingNextLevelName = NextLevelName;
	if (UWorld* World = GetWorld())
	{
		if (NextLevelTravelDelay <= 0.f)
		{
			OpenPendingNextLevel();
		}
		else
		{
			World->GetTimerManager().SetTimer(
				NextLevelTravelTimerHandle,
				this,
				&UTutorialFlowComponent::OpenPendingNextLevel,
				NextLevelTravelDelay,
				false);
		}
	}
}

void UTutorialFlowComponent::OpenPendingNextLevel()
{
	if (PendingNextLevelName.IsNone())
	{
		return;
	}

	const FName LevelName = PendingNextLevelName;
	PendingNextLevelName = NAME_None;
	UE_LOG(LogTutorialFlow, Log, TEXT("OpenPendingNextLevel: Opening level %s."), *LevelName.ToString());
	UGameplayStatics::OpenLevel(this, LevelName);
}

void UTutorialFlowComponent::EvaluateCurrentStepImmediately()
{
	const FTutorialStepDefinition* Step = GetCurrentStep();
	if (!Step)
	{
		return;
	}

	if (Step->TriggerType == ETutorialStepTriggerType::AllAttributeTilesCleared && AreAllAttributeTilesCleared())
	{
		CompleteCurrentStep();
		return;
	}

	if (Step->TriggerType == ETutorialStepTriggerType::AllEnemiesCleared)
	{
		ResolveReferences();
		if (!EnemyManager || EnemyManager->GetAliveEnemies().IsEmpty())
		{
			CompleteCurrentStep();
			return;
		}
	}

	if (Step->TriggerType == ETutorialStepTriggerType::AttributeReached && AttributeComponent)
	{
		const bool bConstructReached = Step->AttributeKind != ETutorialAttributeKind::Acid
			&& AttributeComponent->GetConstructValue() >= Step->RequiredValue;
		const bool bAcidReached = Step->AttributeKind != ETutorialAttributeKind::Construct
			&& AttributeComponent->GetAcidValue() >= Step->RequiredValue;
		if (bConstructReached || bAcidReached)
		{
			CompleteCurrentStep();
			return;
		}
	}

	if (Step->TriggerType == ETutorialStepTriggerType::TransformPieceCollectedCount
		&& CollectedTransformPieceCount >= Step->RequiredValue)
	{
		CompleteCurrentStep();
	}
}

const FTutorialStepDefinition* UTutorialFlowComponent::GetCurrentStep() const
{
	if (!bActive || !FlowData || !FlowData->Steps.IsValidIndex(CurrentStepIndex))
	{
		return nullptr;
	}

	return &FlowData->Steps[CurrentStepIndex];
}

bool UTutorialFlowComponent::IsCurrentStepTrigger(ETutorialStepTriggerType TriggerType) const
{
	const FTutorialStepDefinition* Step = GetCurrentStep();
	return Step && Step->TriggerType == TriggerType;
}

bool UTutorialFlowComponent::AreAllAttributeTilesCleared() const
{
	if (!GridManager)
	{
		return false;
	}

	for (const TPair<FIntPoint, FTileData>& Pair : GridManager->Tiles)
	{
		if (Pair.Value.TileType == ETileType::Construct || Pair.Value.TileType == ETileType::Acid)
		{
			return false;
		}
	}

	return true;
}

bool UTutorialFlowComponent::IsMatchingTransformType(EChessTransformType CandidateType) const
{
	const FTutorialStepDefinition* Step = GetCurrentStep();
	return Step && (Step->TransformType == EChessTransformType::None || Step->TransformType == CandidateType);
}

void UTutorialFlowComponent::HandlePlayerMoved(FIntPoint FromCoord, FIntPoint ToCoord, ETileType EnteredTileType)
{
	if (IsCurrentStepTrigger(ETutorialStepTriggerType::PlayerSteppedOnAnyAttributeTile)
		&& (EnteredTileType == ETileType::Construct || EnteredTileType == ETileType::Acid))
	{
		CompleteCurrentStep();
		return;
	}

	const FTutorialStepDefinition* Step = GetCurrentStep();
	if (Step
		&& Step->TriggerType == ETutorialStepTriggerType::PlayerSteppedOnTileType
		&& Step->TargetTileType == EnteredTileType)
	{
		CompleteCurrentStep();
		return;
	}

	if (IsCurrentStepTrigger(ETutorialStepTriggerType::AllAttributeTilesCleared) && AreAllAttributeTilesCleared())
	{
		CompleteCurrentStep();
	}
}

void UTutorialFlowComponent::HandlePlayerTransformMoveCompleted(UChessPieceFormData* FormData, FIntPoint FromCoord, FIntPoint ToCoord)
{
	if (!IsCurrentStepTrigger(ETutorialStepTriggerType::TransformMoveCompleted) || !FormData)
	{
		return;
	}

	if (IsMatchingTransformType(FormData->TransformType))
	{
		CompleteCurrentStep();
	}
}

void UTutorialFlowComponent::HandlePlayerConversionEnergyUsed(ETileType EnergyType, bool bConvertedAny)
{
	if (bConvertedAny && IsCurrentStepTrigger(ETutorialStepTriggerType::ConversionEnergyUsed))
	{
		CompleteCurrentStep();
	}
}

void UTutorialFlowComponent::HandlePlayerAttributeChanged(int32 NewConstructValue, int32 NewAcidValue)
{
	const FTutorialStepDefinition* Step = GetCurrentStep();
	if (!Step || Step->TriggerType != ETutorialStepTriggerType::AttributeReached)
	{
		return;
	}

	const bool bConstructReached = Step->AttributeKind != ETutorialAttributeKind::Acid && NewConstructValue >= Step->RequiredValue;
	const bool bAcidReached = Step->AttributeKind != ETutorialAttributeKind::Construct && NewAcidValue >= Step->RequiredValue;
	if (bConstructReached || bAcidReached)
	{
		CompleteCurrentStep();
	}
}

void UTutorialFlowComponent::HandlePlayerHealthChanged(int32 NewHealth, int32 MaxHealth)
{
	const bool bDamaged = LastObservedHealth > 0 && NewHealth < LastObservedHealth;
	LastObservedHealth = NewHealth;
	if (bDamaged && IsCurrentStepTrigger(ETutorialStepTriggerType::PlayerDamagedOrEnemyKilled))
	{
		CompleteCurrentStep();
	}
}

void UTutorialFlowComponent::HandleConversionEnergyChanged(bool bHasEnergy, ETileType EnergyType)
{
}

void UTutorialFlowComponent::HandleTileTypeChanged(FIntPoint TileCoord, ETileType NewTileType)
{
	if (IsCurrentStepTrigger(ETutorialStepTriggerType::AllAttributeTilesCleared) && AreAllAttributeTilesCleared())
	{
		CompleteCurrentStep();
	}
}

void UTutorialFlowComponent::HandleEnemyKilled(AGridEnemyPawn* Enemy, FIntPoint DeathCoord, FVector DeathWorldLocation)
{
	if (IsCurrentStepTrigger(ETutorialStepTriggerType::EnemyKilled)
		|| IsCurrentStepTrigger(ETutorialStepTriggerType::PlayerDamagedOrEnemyKilled))
	{
		CompleteCurrentStep();
		return;
	}

	if (IsCurrentStepTrigger(ETutorialStepTriggerType::AllEnemiesCleared))
	{
		ResolveReferences();
		if (!EnemyManager || EnemyManager->GetAliveEnemies().IsEmpty())
		{
			CompleteCurrentStep();
		}
	}
}

void UTutorialFlowComponent::HandleAllEnemiesCleared()
{
	if (IsCurrentStepTrigger(ETutorialStepTriggerType::AllEnemiesCleared))
	{
		CompleteCurrentStep();
	}
}

void UTutorialFlowComponent::HandlePickupCollected(AGridPickupActor* Pickup, AGridPawn* CollectingPawn, bool bEffectApplied)
{
	if (!Pickup || CollectingPawn != PlayerPawn)
	{
		return;
	}

	if (Cast<AGridTransformPiecePickupActor>(Pickup))
	{
		++CollectedTransformPieceCount;
	}

	const FTutorialStepDefinition* Step = GetCurrentStep();
	if (Step
		&& Step->TriggerType == ETutorialStepTriggerType::TransformPieceCollectedCount
		&& CollectedTransformPieceCount >= Step->RequiredValue)
	{
		CompleteCurrentStep();
	}
}

void UTutorialFlowComponent::HandleTransformInventoryChanged()
{
}

void UTutorialFlowComponent::HandleTransformWheelOpened()
{
	if (IsCurrentStepTrigger(ETutorialStepTriggerType::TransformWheelOpened))
	{
		CompleteCurrentStep();
	}
}

void UTutorialFlowComponent::HandleTransformSelected(UChessPieceFormData* FormData)
{
}
