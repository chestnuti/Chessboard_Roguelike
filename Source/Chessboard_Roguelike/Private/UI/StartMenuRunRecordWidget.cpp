#include "UI/StartMenuRunRecordWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Save/RunRecordBlueprintLibrary.h"

void UStartMenuRunRecordWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildFallbackWidgetTreeIfNeeded();
	RefreshRecordDisplay();
}

void UStartMenuRunRecordWidget::RefreshRecordDisplay()
{
	const FRunRecordStats Stats = URunRecordBlueprintLibrary::LoadRunRecordStats(this);

	if (HighestLevelText)
	{
		HighestLevelText->SetText(Stats.bHasRecord
			? FText::Format(
				NSLOCTEXT("StartMenuRunRecord", "HighestLevelFormat", "Farthest Level: {0}"),
				FText::AsNumber(Stats.HighestLevelReached))
			: NSLOCTEXT("StartMenuRunRecord", "NoHighestLevel", "Farthest Level: None"));
	}

	if (BestTimeText)
	{
		BestTimeText->SetText(Stats.bHasRecord
			? FText::Format(
				NSLOCTEXT("StartMenuRunRecord", "BestTimeFormat", "Fastest Time: {0}"),
				URunRecordBlueprintLibrary::FormatRunRecordTime(Stats.BestTimeToHighestLevelSeconds))
			: NSLOCTEXT("StartMenuRunRecord", "NoBestTime", "Fastest Time: --:--"));
	}
}

void UStartMenuRunRecordWidget::BuildFallbackWidgetTreeIfNeeded()
{
	if (!WidgetTree || HighestLevelText || BestTimeText)
	{
		return;
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RunRecordRoot"));
	WidgetTree->RootWidget = RootBox;

	HighestLevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HighestLevelText"));
	BestTimeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BestTimeText"));

	if (HighestLevelText)
	{
		HighestLevelText->SetText(NSLOCTEXT("StartMenuRunRecord", "HighestLevelInitialText", "Farthest Level: None"));
		RootBox->AddChildToVerticalBox(HighestLevelText);
	}

	if (BestTimeText)
	{
		BestTimeText->SetText(NSLOCTEXT("StartMenuRunRecord", "BestTimeInitialText", "Fastest Time: --:--"));
		RootBox->AddChildToVerticalBox(BestTimeText);
	}
}
