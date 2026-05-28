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
	SetTurnState(ETurnState::PlayerActionResolve);
}

void ATurnManager::EndPlayerAction()
{
	SetTurnState(ETurnState::PlayerInput);
}

void ATurnManager::AddStep()
{
	++StepCount;
	UE_LOG(LogTurnManager, Log, TEXT("StepCount = %d"), StepCount);
}
