#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialInstructionWidget.generated.h"

class UTextBlock;

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API UTutorialInstructionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void SetInstruction(const FText& InInstructionText, int32 CurrentStepNumber, int32 TotalStepCount);

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void ClearInstruction();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial")
	TObjectPtr<UTextBlock> StepCounterText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Tutorial")
	TObjectPtr<UTextBlock> InstructionText;

private:
	FText PendingInstructionText;
	int32 PendingCurrentStepNumber = 0;
	int32 PendingTotalStepCount = 0;

	void BuildFallbackWidgetTreeIfNeeded();
	void RefreshDisplay();
};
