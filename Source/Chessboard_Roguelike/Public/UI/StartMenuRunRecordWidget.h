#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StartMenuRunRecordWidget.generated.h"

class UTextBlock;

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API UStartMenuRunRecordWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Run Record")
	void RefreshRecordDisplay();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Run Record")
	TObjectPtr<UTextBlock> HighestLevelText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Run Record")
	TObjectPtr<UTextBlock> BestTimeText;

private:
	void BuildFallbackWidgetTreeIfNeeded();
};
