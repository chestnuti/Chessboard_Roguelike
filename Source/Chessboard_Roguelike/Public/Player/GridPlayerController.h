#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GridPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API AGridPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> GridMovementMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveUpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveDownAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveLeftAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveRightAction;

private:
	void MoveUp();
	void MoveDown();
	void MoveLeft();
	void MoveRight();
	void RequestPawnMove(FIntPoint Direction);
};
