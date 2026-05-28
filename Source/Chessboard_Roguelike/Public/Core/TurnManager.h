#pragma once

#include "CoreMinimal.h"
#include "Core/TurnStateTypes.h"
#include "GameFramework/Actor.h"
#include "TurnManager.generated.h"

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API ATurnManager : public AActor
{
	GENERATED_BODY()

public:
	ATurnManager();

	UPROPERTY(BlueprintReadOnly, Category = "Turn")
	ETurnState CurrentTurnState = ETurnState::Initializing;

	UPROPERTY(BlueprintReadOnly, Category = "Turn")
	int32 StepCount = 0;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn")
	bool CanAcceptPlayerInput() const;

	UFUNCTION(BlueprintCallable, Category = "Turn")
	void SetTurnState(ETurnState NewState);

	UFUNCTION(BlueprintCallable, Category = "Turn")
	void BeginPlayerAction();

	UFUNCTION(BlueprintCallable, Category = "Turn")
	void EndPlayerAction();

	UFUNCTION(BlueprintCallable, Category = "Turn")
	void AddStep();
};
