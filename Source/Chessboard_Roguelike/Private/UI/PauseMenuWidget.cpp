#include "UI/PauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

namespace
{
UButton* CreateFallbackButton(UWidgetTree* WidgetTree, UPanelWidget* RootPanel, const FName& ButtonName, const FText& LabelText)
{
	if (!WidgetTree || !RootPanel)
	{
		return nullptr;
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(ButtonName.ToString() + TEXT("Label")));
	if (!Button)
	{
		return nullptr;
	}

	if (Label)
	{
		Label->SetText(LabelText);
		Button->AddChild(Label);
	}

	RootPanel->AddChild(Button);
	return Button;
}
}

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildFallbackWidgetTreeIfNeeded();

	if (ResumeButton)
	{
		ResumeButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleResumeClicked);
		ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleResumeClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleBackClicked);
		BackButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleBackClicked);
	}
	if (Back)
	{
		Back->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleBackClicked);
		Back->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleBackClicked);
	}
	if (SettingButton)
	{
		SettingButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleSettingClicked);
		SettingButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleSettingClicked);
	}
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleMainMenuClicked);
		MainMenuButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleMainMenuClicked);
	}
	if (QuitGameButton)
	{
		QuitGameButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleQuitGameClicked);
		QuitGameButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleQuitGameClicked);
	}
}

void UPauseMenuWidget::NativeDestruct()
{
	if (ResumeButton)
	{
		ResumeButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleResumeClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleBackClicked);
	}
	if (Back)
	{
		Back->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleBackClicked);
	}
	if (SettingButton)
	{
		SettingButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleSettingClicked);
	}
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleMainMenuClicked);
	}
	if (QuitGameButton)
	{
		QuitGameButton->OnClicked.RemoveDynamic(this, &UPauseMenuWidget::HandleQuitGameClicked);
	}

	Super::NativeDestruct();
}

void UPauseMenuWidget::HandleResumeClicked()
{
	RemoveFromParent();
	OnResumeRequested.Broadcast();
}

void UPauseMenuWidget::HandleBackClicked()
{
	OnBackRequested.Broadcast();
}

void UPauseMenuWidget::HandleSettingClicked()
{
	OnSettingsRequested.Broadcast();
}

void UPauseMenuWidget::HandleMainMenuClicked()
{
	RemoveFromParent();
	OnMainMenuRequested.Broadcast();
}

void UPauseMenuWidget::HandleQuitGameClicked()
{
	OnQuitGameRequested.Broadcast();
}

void UPauseMenuWidget::BuildFallbackWidgetTreeIfNeeded()
{
	if (!WidgetTree)
	{
		return;
	}

	UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget);
	if (!RootPanel)
	{
		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseMenuRoot"));
		WidgetTree->RootWidget = RootBox;
		RootPanel = RootBox;
	}

	if (!ResumeButton)
	{
		ResumeButton = CreateFallbackButton(WidgetTree, RootPanel, TEXT("ResumeButton"), NSLOCTEXT("PauseMenu", "ResumeButton", "Resume"));
	}
	if (!BackButton)
	{
		BackButton = CreateFallbackButton(WidgetTree, RootPanel, TEXT("BackButton"), NSLOCTEXT("PauseMenu", "BackButton", "Back"));
	}
	if (!SettingButton)
	{
		SettingButton = CreateFallbackButton(WidgetTree, RootPanel, TEXT("SettingButton"), NSLOCTEXT("PauseMenu", "SettingButton", "Setting"));
	}
	if (!MainMenuButton)
	{
		MainMenuButton = CreateFallbackButton(WidgetTree, RootPanel, TEXT("MainMenuButton"), NSLOCTEXT("PauseMenu", "MainMenuButton", "Main Menu"));
	}
	if (!QuitGameButton)
	{
		QuitGameButton = CreateFallbackButton(WidgetTree, RootPanel, TEXT("QuitGameButton"), NSLOCTEXT("PauseMenu", "QuitGameButton", "Quit Game"));
	}
}
