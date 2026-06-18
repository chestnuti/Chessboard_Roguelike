#include "UI/TutorialInstructionWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateColor.h"

void UTutorialInstructionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackWidgetTreeIfNeeded();
	RefreshDisplay();
}

void UTutorialInstructionWidget::SetInstruction(const FText& InInstructionText, int32 CurrentStepNumber, int32 TotalStepCount)
{
	PendingInstructionText = InInstructionText;
	PendingCurrentStepNumber = CurrentStepNumber;
	PendingTotalStepCount = TotalStepCount;
	RefreshDisplay();
}

void UTutorialInstructionWidget::ClearInstruction()
{
	PendingInstructionText = FText::GetEmpty();
	PendingCurrentStepNumber = 0;
	PendingTotalStepCount = 0;
	RefreshDisplay();
}

void UTutorialInstructionWidget::BuildFallbackWidgetTreeIfNeeded()
{
	if (!WidgetTree || InstructionText || StepCounterText)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TutorialRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TutorialPanel"));
	PanelBorder->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.03f, 0.82f));
	PanelBorder->SetPadding(FMargin(18.f, 14.f));
	RootCanvas->AddChild(PanelBorder);

	if (UCanvasPanelSlot* BorderSlot = Cast<UCanvasPanelSlot>(PanelBorder->Slot))
	{
		BorderSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		BorderSlot->SetAlignment(FVector2D(0.f, 0.f));
		BorderSlot->SetPosition(FVector2D(28.f, 28.f));
		BorderSlot->SetSize(FVector2D(560.f, 150.f));
	}

	UVerticalBox* TextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TutorialTextBox"));
	PanelBorder->SetContent(TextBox);

	StepCounterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StepCounterText"));
	StepCounterText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.80f, 1.f, 1.f)));
	StepCounterText->SetAutoWrapText(true);
	TextBox->AddChildToVerticalBox(StepCounterText);

	InstructionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InstructionText"));
	InstructionText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	InstructionText->SetAutoWrapText(true);
	InstructionText->SetJustification(ETextJustify::Left);
	if (UVerticalBoxSlot* InstructionSlot = TextBox->AddChildToVerticalBox(InstructionText))
	{
		InstructionSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
}

void UTutorialInstructionWidget::RefreshDisplay()
{
	if (StepCounterText)
	{
		if (PendingCurrentStepNumber > 0 && PendingTotalStepCount > 0)
		{
			StepCounterText->SetText(FText::Format(
				NSLOCTEXT("TutorialInstruction", "StepCounterFormat", "Tutorial {0} / {1}"),
				FText::AsNumber(PendingCurrentStepNumber),
				FText::AsNumber(PendingTotalStepCount)));
		}
		else
		{
			StepCounterText->SetText(FText::GetEmpty());
		}
	}

	if (InstructionText)
	{
		InstructionText->SetText(PendingInstructionText);
	}
}
