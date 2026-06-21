#include "Core/TurnManager.h"

#include "Audio/GameAudioSubsystem.h"
#include "Engine/GameInstance.h"

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

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
		{
			if (CurrentTurnState == ETurnState::Victory)
			{
				AudioSubsystem->PlayLevelVictorySFX();
			}
			else if (CurrentTurnState == ETurnState::Defeat)
			{
				AudioSubsystem->PlayLevelFailedSFX();
			}
		}
	}
}

void ATurnManager::BeginPlayerAction()
{
	// Enter the action-resolve lock before gameplay state changes that should reject input.
	SetTurnState(ETurnState::PlayerActionResolve);
}

void ATurnManager::EndPlayerAction()
{
	if (CurrentTurnState == ETurnState::Victory || CurrentTurnState == ETurnState::Defeat)
	{
		return;
	}

	// Return control only after the current action has fully resolved.
	SetTurnState(ETurnState::PlayerInput);
}

void ATurnManager::BeginEnemyTurn()
{
	SetTurnState(ETurnState::EnemyTurnResolve);
}

void ATurnManager::EndEnemyTurn()
{
	if (CurrentTurnState == ETurnState::Victory || CurrentTurnState == ETurnState::Defeat)
	{
		return;
	}

	SetTurnState(ETurnState::PlayerInput);
}

void ATurnManager::AddStep()
{
	++StepCount;
	UE_LOG(LogTurnManager, Log, TEXT("StepCount = %d"), StepCount);
}
