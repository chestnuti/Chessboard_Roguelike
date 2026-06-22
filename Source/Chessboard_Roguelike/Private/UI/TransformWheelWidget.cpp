#include "UI/TransformWheelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/ChessPieceFormData.h"
#include "Player/GridPlayerController.h"
#include "Player/PlayerTransformInventoryComponent.h"

namespace
{
	constexpr int32 TransformWheelSlotCount = 4;

	const FVector2D SlotPositions[TransformWheelSlotCount] =
	{
		FVector2D(0.f, -120.f),
		FVector2D(140.f, 0.f),
		FVector2D(0.f, 120.f),
		FVector2D(-140.f, 0.f)
	};
}

void UTransformWheelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheDesignedSlotWidgets();
	BuildDefaultWheelIfNeeded();
	BindSlotButtonDelegates();
	RefreshSlotLabels();
}

void UTransformWheelWidget::InitializeWheel(AGridPlayerController* InOwningGridController)
{
	OwningGridController = InOwningGridController;
	if (OwningGridController)
	{
		SlotForms = OwningGridController->GetTransformWheelForms();
	}

	CacheDesignedSlotWidgets();
	BuildDefaultWheelIfNeeded();
	BindSlotButtonDelegates();
	RefreshSlotLabels();
	OnWheelInitialized();
}

void UTransformWheelWidget::RefreshFromInventory(UPlayerTransformInventoryComponent* InventoryComponent)
{
	CachedInventoryComponent = InventoryComponent;
	CacheDesignedSlotWidgets();
	BuildDefaultWheelIfNeeded();
	BindSlotButtonDelegates();
	RefreshSlotLabels();
	OnInventoryRefreshed(InventoryComponent);
}

void UTransformWheelWidget::RequestSelectTransform(UChessPieceFormData* FormData)
{

	if (OwningGridController)
	{
		OwningGridController->RequestSelectTransform(FormData);
	}
}

AGridPlayerController* UTransformWheelWidget::GetOwningGridController() const
{
	return OwningGridController;
}

void UTransformWheelWidget::HandleSlot0Clicked()
{
	HandleSlotClicked(0);
}

void UTransformWheelWidget::HandleSlot1Clicked()
{
	HandleSlotClicked(1);
}

void UTransformWheelWidget::HandleSlot2Clicked()
{
	HandleSlotClicked(2);
}

void UTransformWheelWidget::HandleSlot3Clicked()
{
	HandleSlotClicked(3);
}

void UTransformWheelWidget::BuildDefaultWheelIfNeeded()
{
	if (!WidgetTree || WidgetTree->RootWidget || SlotButtons.Num() >= TransformWheelSlotCount)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TransformWheelRoot"));
	WidgetTree->RootWidget = RootCanvas;

	SlotButtons.Reset();
	SlotIcons.Reset();
	SlotLabels.Reset();
	SlotCountLabels.Reset();

	for (int32 SlotIndex = 0; SlotIndex < TransformWheelSlotCount; ++SlotIndex)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("TransformSlot_%d"), SlotIndex));
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("TransformSlotLabel_%d"), SlotIndex));
		if (!Button || !Label)
		{
			continue;
		}

		Label->SetJustification(ETextJustify::Center);
		Button->AddChild(Label);
		RootCanvas->AddChild(Button);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Button->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetPosition(SlotPositions[SlotIndex]);
			CanvasSlot->SetSize(FVector2D(160.f, 72.f));
		}

		switch (SlotIndex)
		{
		case 0:
			Button->OnClicked.AddDynamic(this, &UTransformWheelWidget::HandleSlot0Clicked);
			break;
		case 1:
			Button->OnClicked.AddDynamic(this, &UTransformWheelWidget::HandleSlot1Clicked);
			break;
		case 2:
			Button->OnClicked.AddDynamic(this, &UTransformWheelWidget::HandleSlot2Clicked);
			break;
		case 3:
			Button->OnClicked.AddDynamic(this, &UTransformWheelWidget::HandleSlot3Clicked);
			break;
		default:
			break;
		}

		SlotButtons.Add(Button);
		SlotIcons.Add(nullptr);
		SlotLabels.Add(Label);
		SlotCountLabels.Add(nullptr);
	}
}

void UTransformWheelWidget::CacheDesignedSlotWidgets()
{
	const bool bHasDesignedSlots = Slot_Knight || Slot_Bishop || Slot_Rook || Slot_Queen;
	if (!bHasDesignedSlots)
	{
		return;
	}

	SlotButtons.Reset();
	SlotButtons.Add(Slot_Knight);
	SlotButtons.Add(Slot_Bishop);
	SlotButtons.Add(Slot_Rook);
	SlotButtons.Add(Slot_Queen);

	SlotIcons.Reset();
	SlotIcons.Add(Icon_Knight);
	SlotIcons.Add(Icon_Bishop);
	SlotIcons.Add(Icon_Rook);
	SlotIcons.Add(Icon_Queen);

	SlotLabels.Reset();
	SlotLabels.Add(NameText_Knight);
	SlotLabels.Add(NameText_Bishop);
	SlotLabels.Add(NameText_Rook);
	SlotLabels.Add(NameText_Queen);

	SlotCountLabels.Reset();
	SlotCountLabels.Add(CountText_Knight);
	SlotCountLabels.Add(CountText_Bishop);
	SlotCountLabels.Add(CountText_Rook);
	SlotCountLabels.Add(CountText_Queen);
}

void UTransformWheelWidget::BindSlotButtonDelegates()
{
	for (UButton* Button : SlotButtons)
	{
		if (Button)
		{
			Button->OnClicked.RemoveDynamic(this, &UTransformWheelWidget::HandleSlot0Clicked);
			Button->OnClicked.RemoveDynamic(this, &UTransformWheelWidget::HandleSlot1Clicked);
			Button->OnClicked.RemoveDynamic(this, &UTransformWheelWidget::HandleSlot2Clicked);
			Button->OnClicked.RemoveDynamic(this, &UTransformWheelWidget::HandleSlot3Clicked);
		}
	}

	if (SlotButtons.IsValidIndex(0) && SlotButtons[0])
	{
		SlotButtons[0]->OnClicked.AddDynamic(this, &UTransformWheelWidget::HandleSlot0Clicked);
	}
	if (SlotButtons.IsValidIndex(1) && SlotButtons[1])
	{
		SlotButtons[1]->OnClicked.AddDynamic(this, &UTransformWheelWidget::HandleSlot1Clicked);
	}
	if (SlotButtons.IsValidIndex(2) && SlotButtons[2])
	{
		SlotButtons[2]->OnClicked.AddDynamic(this, &UTransformWheelWidget::HandleSlot2Clicked);
	}
	if (SlotButtons.IsValidIndex(3) && SlotButtons[3])
	{
		SlotButtons[3]->OnClicked.AddDynamic(this, &UTransformWheelWidget::HandleSlot3Clicked);
	}
}

void UTransformWheelWidget::HandleSlotClicked(int32 SlotIndex)
{
	if (!SlotForms.IsValidIndex(SlotIndex))
	{
		return;
	}

	RequestSelectTransform(SlotForms[SlotIndex]);
}

void UTransformWheelWidget::RefreshSlotLabels()
{
	if (OwningGridController && SlotForms.IsEmpty())
	{
		SlotForms = OwningGridController->GetTransformWheelForms();
	}

	for (int32 SlotIndex = 0; SlotIndex < SlotLabels.Num(); ++SlotIndex)
	{
		UChessPieceFormData* FormData = SlotForms.IsValidIndex(SlotIndex) ? SlotForms[SlotIndex] : nullptr;
		const int32 Count = (CachedInventoryComponent && FormData)
			? CachedInventoryComponent->GetTransformPieceCount(FormData->TransformType)
			: 0;

		const FText DisplayName = FormData && !FormData->DisplayName.IsEmpty()
			? FormData->DisplayName
			: FText::FromString(TEXT("Empty"));
		const FText LabelText = FText::Format(
			NSLOCTEXT("TransformWheel", "SlotLabelFormat", "{0}\n{1}"),
			DisplayName,
			FText::AsNumber(Count));

		if (SlotLabels[SlotIndex])
		{
			SlotLabels[SlotIndex]->SetText(LabelText);
		}

		if (SlotCountLabels.IsValidIndex(SlotIndex) && SlotCountLabels[SlotIndex])
		{
			SlotCountLabels[SlotIndex]->SetText(FText::AsNumber(Count));
		}

		if (SlotIcons.IsValidIndex(SlotIndex) && SlotIcons[SlotIndex])
		{
			UTexture2D* IconTexture = FormData ? FormData->Icon.Get() : nullptr;
			SlotIcons[SlotIndex]->SetBrushFromTexture(IconTexture, true);
			SlotIcons[SlotIndex]->SetVisibility(IconTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}

		if (SlotButtons.IsValidIndex(SlotIndex) && SlotButtons[SlotIndex])
		{
			SlotButtons[SlotIndex]->SetIsEnabled(FormData && Count > 0);
		}
	}
}
