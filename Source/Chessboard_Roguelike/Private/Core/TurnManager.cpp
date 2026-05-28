#include "Core/TurnManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogTurnManager, Log, All);

ATurnManager::ATurnManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool ATurnManager::CanAcceptPlayerInput() const
{
	return CurrentTurnState == ETurnState::PlayerInput;
}

void ATurnManager::SetTurnState(ETurnState NewState)
{
	if (CurrentTurnState == NewState)
	{
		return;
	}

	UE_LOG(LogTurnManager, Log, TEXT("Turn state changed: %d -> %d"),
		static_cast<uint8>(CurrentTurnState), static_cast<uint8>(NewState));
	CurrentTurnState = NewState;
}

void ATurnManager::BeginPlayerAction()
{
	// Enter the action-resolve lock before gameplay state changes that should reject input.
	SetTurnState(ETurnState::PlayerActionResolve);
}

void ATurnManager::EndPlayerAction()
{
	// Return control only after the current action has fully resolved.
	SetTurnState(ETurnState::PlayerInput);
}

void ATurnManager::AddStep()
{
	++StepCount;
	UE_LOG(LogTurnManager, Log, TEXT("StepCount = %d"), StepCount);
}
