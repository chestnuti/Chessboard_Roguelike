#include "UI/SettingsMenuWidget.h"

#include "Audio/GameAudioSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"

void USettingsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildFallbackWidgetTreeIfNeeded();

	if (BGMSlider)
	{
		BGMSlider->OnValueChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleBGMVolumeChanged);
		BGMSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleBGMVolumeChanged);
	}

	if (SFXSlider)
	{
		SFXSlider->OnValueChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleSFXVolumeChanged);
		SFXSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleSFXVolumeChanged);
	}

	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &USettingsMenuWidget::HandleBackClicked);
		BackButton->OnClicked.AddDynamic(this, &USettingsMenuWidget::HandleBackClicked);
	}

	RefreshFromAudioSettings();
}

void USettingsMenuWidget::NativeDestruct()
{
	if (BGMSlider)
	{
		BGMSlider->OnValueChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleBGMVolumeChanged);
	}

	if (SFXSlider)
	{
		SFXSlider->OnValueChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleSFXVolumeChanged);
	}

	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &USettingsMenuWidget::HandleBackClicked);
	}

	Super::NativeDestruct();
}

void USettingsMenuWidget::RefreshFromAudioSettings()
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UGameAudioSubsystem* AudioSubsystem = GameInstance
		? GameInstance->GetSubsystem<UGameAudioSubsystem>()
		: nullptr;

	const float BGMVolume = AudioSubsystem ? AudioSubsystem->GetBGMVolume() : 1.0f;
	const float SFXVolume = AudioSubsystem ? AudioSubsystem->GetSFXVolume() : 1.0f;
	SetSliderValueSilently(BGMSlider, BGMVolume);
	SetSliderValueSilently(SFXSlider, SFXVolume);
	RefreshVolumeLabels();
}

void USettingsMenuWidget::HandleBGMVolumeChanged(float Value)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
		{
			AudioSubsystem->SetBGMVolume(Value);
		}
	}

	RefreshVolumeLabels();
}

void USettingsMenuWidget::HandleSFXVolumeChanged(float Value)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UGameAudioSubsystem* AudioSubsystem = GameInstance->GetSubsystem<UGameAudioSubsystem>())
		{
			AudioSubsystem->SetSFXVolume(Value);
		}
	}

	RefreshVolumeLabels();
}

void USettingsMenuWidget::HandleBackClicked()
{
	OnSettingsBackRequested.Broadcast();
}

void USettingsMenuWidget::BuildFallbackWidgetTreeIfNeeded()
{
	if (!WidgetTree)
	{
		return;
	}

	UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget);
	if (!RootPanel)
	{
		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsRoot"));
		WidgetTree->RootWidget = RootBox;
		RootPanel = RootBox;
	}

	if (!BGMVolumeText)
	{
		BGMVolumeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BGMVolumeText"));
		if (BGMVolumeText)
		{
			RootPanel->AddChild(BGMVolumeText);
		}
	}

	if (!BGMSlider)
	{
		BGMSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("BGMSlider"));
		if (BGMSlider)
		{
			RootPanel->AddChild(BGMSlider);
		}
	}

	if (!SFXVolumeText)
	{
		SFXVolumeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SFXVolumeText"));
		if (SFXVolumeText)
		{
			RootPanel->AddChild(SFXVolumeText);
		}
	}

	if (!SFXSlider)
	{
		SFXSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("SFXSlider"));
		if (SFXSlider)
		{
			RootPanel->AddChild(SFXSlider);
		}
	}

	if (!BackButton)
	{
		BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackButton"));
		UTextBlock* BackLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackButtonLabel"));
		if (BackButton && BackLabel)
		{
			BackLabel->SetText(NSLOCTEXT("SettingsMenu", "BackButtonLabel", "Back"));
			BackButton->AddChild(BackLabel);
		}
		if (BackButton)
		{
			RootPanel->AddChild(BackButton);
		}
	}
}

void USettingsMenuWidget::SetSliderValueSilently(USlider* Slider, float Value)
{
	if (!Slider)
	{
		return;
	}

	Slider->SetValue(FMath::Clamp(Value, 0.0f, 1.0f));
}

void USettingsMenuWidget::RefreshVolumeLabels()
{
	if (BGMVolumeText)
	{
		const int32 VolumePercent = FMath::RoundToInt((BGMSlider ? BGMSlider->GetValue() : 1.0f) * 100.0f);
		BGMVolumeText->SetText(FText::Format(
			NSLOCTEXT("SettingsMenu", "BGMVolumeFormat", "BGM Volume: {0}%"),
			FText::AsNumber(VolumePercent)));
	}

	if (SFXVolumeText)
	{
		const int32 VolumePercent = FMath::RoundToInt((SFXSlider ? SFXSlider->GetValue() : 1.0f) * 100.0f);
		SFXVolumeText->SetText(FText::Format(
			NSLOCTEXT("SettingsMenu", "SFXVolumeFormat", "SFX Volume: {0}%"),
			FText::AsNumber(VolumePercent)));
	}
}
