#include "UI/PlayerAttributeHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Enemy/GridEnemyManager.h"
#include "PCG/DungeonRunManager.h"
#include "Player/ConversionEnergyComponent.h"
#include "Player/PlayerAttributeComponent.h"
#include "UI/SettingsMenuWidget.h"

namespace
{
constexpr float AcidEnemyTypeValue = 0.f;
constexpr float ConstructEnemyTypeValue = 1.f;

const FName EnemyTypeParameterName(TEXT("EnemyType"));
const FName FullParameterName(TEXT("Full"));
const FName OffsetXParameterName(TEXT("Offset_X"));

FText GetConversionEnergyTypeDisplayName(ETileType EnergyType)
{
	switch (EnergyType)
	{
	case ETileType::Construct:
		return NSLOCTEXT("PlayerAttributeHUD", "ConstructEnergyDisplayName", "Construct");
	case ETileType::Acid:
		return NSLOCTEXT("PlayerAttributeHUD", "AcidEnergyDisplayName", "Acid");
	default:
		return NSLOCTEXT("PlayerAttributeHUD", "NoEnergyDisplayName", "None");
	}
}
}

void UPlayerAttributeHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!SettingsMenuClass)
	{
		SettingsMenuClass = USettingsMenuWidget::StaticClass();
	}

	// Gives the native class a minimal visible HUD even if the Widget Blueprint has no designer tree yet.
	BuildFallbackWidgetTreeIfNeeded();

	if (SettingButton)
	{
		SettingButton->OnClicked.RemoveDynamic(this, &UPlayerAttributeHUDWidget::HandleSettingButtonClicked);
		SettingButton->OnClicked.AddDynamic(this, &UPlayerAttributeHUDWidget::HandleSettingButtonClicked);
	}

	if (!AttributeComponent)
	{
		if (APawn* OwningPawn = GetOwningPlayerPawn())
		{
			BindToAttributeComponent(OwningPawn->FindComponentByClass<UPlayerAttributeComponent>());
		}
	}

	if (!ConversionEnergyComponent)
	{
		if (APawn* OwningPawn = GetOwningPlayerPawn())
		{
			BindToConversionEnergyComponent(OwningPawn->FindComponentByClass<UConversionEnergyComponent>());
		}
	}

	RefreshAttributeDisplay();
	RefreshConversionEnergyDisplay();
	RefreshDungeonDisplay();
	RefreshEnemyCountDisplay();
	StartDungeonRunTimerRefresh();
}

void UPlayerAttributeHUDWidget::NativeDestruct()
{
	CloseSettingsMenu();
	if (SettingButton)
	{
		SettingButton->OnClicked.RemoveDynamic(this, &UPlayerAttributeHUDWidget::HandleSettingButtonClicked);
	}

	StopDungeonRunTimerRefresh();
	UnbindFromEnemyManager();
	UnbindFromDungeonRunManager();
	UnbindFromConversionEnergyComponent();
	UnbindFromAttributeComponent();
	Super::NativeDestruct();
}

void UPlayerAttributeHUDWidget::InitializeFromAttributeComponent(UPlayerAttributeComponent* InAttributeComponent)
{
	BindToAttributeComponent(InAttributeComponent);
	RefreshAttributeDisplay();
}

void UPlayerAttributeHUDWidget::InitializeFromConversionEnergyComponent(UConversionEnergyComponent* InEnergyComponent)
{
	BindToConversionEnergyComponent(InEnergyComponent);
	RefreshConversionEnergyDisplay();
}

void UPlayerAttributeHUDWidget::InitializeFromDungeonRunManager(ADungeonRunManager* InDungeonRunManager)
{
	BindToDungeonRunManager(InDungeonRunManager);
	RefreshDungeonDisplay();
	StartDungeonRunTimerRefresh();
}

void UPlayerAttributeHUDWidget::RefreshAttributeDisplay()
{
	if (!AttributeComponent)
	{
		return;
	}

	// Pull current values from the component during initialization and whenever its change event fires.
	if (HealthText)
	{
		HealthText->SetText(FText::Format(
			NSLOCTEXT("PlayerAttributeHUD", "HealthValueFormat", "HP: {0} / {1}"),
			FText::AsNumber(AttributeComponent->GetCurrentHealth()),
			FText::AsNumber(AttributeComponent->GetMaxHealth())));
	}

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(AttributeComponent->GetHealthRatio());
	}

	if (ConstructText)
	{
		ConstructText->SetText(FText::Format(
			NSLOCTEXT("PlayerAttributeHUD", "ConstructValueFormat", "Construct: {0} / {1}"),
			FText::AsNumber(AttributeComponent->GetConstructValue()),
			FText::AsNumber(AttributeComponent->GetMaxConstructValue())));
	}

	if (AcidText)
	{
		AcidText->SetText(FText::Format(
			NSLOCTEXT("PlayerAttributeHUD", "AcidValueFormat", "Acid: {0} / {1}"),
			FText::AsNumber(AttributeComponent->GetAcidValue()),
			FText::AsNumber(AttributeComponent->GetMaxAcidValue())));
	}

	if (ConstructProgressBar)
	{
		ConstructProgressBar->SetPercent(AttributeComponent->GetConstructRatio());
	}

	if (AcidProgressBar)
	{
		AcidProgressBar->SetPercent(AttributeComponent->GetAcidRatio());
	}

	ConstructAttributeMaterial = RefreshAttributeImage(
		ConstructAttribute,
		ConstructAttributeMaterial,
		AttributeComponent->GetConstructValue(),
		AttributeComponent->GetMaxConstructValue(),
		ConstructEnemyTypeValue);

	AcidAttributeMaterial = RefreshAttributeImage(
		AcidAttribute,
		AcidAttributeMaterial,
		AttributeComponent->GetAcidValue(),
		AttributeComponent->GetMaxAcidValue(),
		AcidEnemyTypeValue);
}

void UPlayerAttributeHUDWidget::RefreshConversionEnergyDisplay()
{
	if (EnergyText)
	{
		EnergyText->SetText(GetConversionEnergyStatusText());
	}
}

void UPlayerAttributeHUDWidget::RefreshDungeonDisplay()
{
	if (LevelText)
	{
		const int32 LevelIndex = DungeonRunManager ? DungeonRunManager->GetCurrentLevelDisplayIndex() : 1;
		LevelText->SetText(FText::Format(
			NSLOCTEXT("PlayerAttributeHUD", "DungeonLevelFormat", "Level: {0}"),
			FText::AsNumber(LevelIndex)));
	}

	if (TimerText)
	{
		TimerText->SetText(GetDungeonTimerText());
	}
}

void UPlayerAttributeHUDWidget::RefreshEnemyCountDisplay()
{
	if (!EnemyCountText)
	{
		return;
	}

	const int32 AliveEnemyCount = EnemyManager ? EnemyManager->GetAliveEnemies().Num() : 0;
	EnemyCountText->SetText(FText::Format(
		NSLOCTEXT("PlayerAttributeHUD", "EnemyCountFormat", "Enemies: {0}"),
		FText::AsNumber(AliveEnemyCount)));
}

FText UPlayerAttributeHUDWidget::GetConversionEnergyStatusText() const
{
	if (!ConversionEnergyComponent || !ConversionEnergyComponent->HasConversionEnergy())
	{
		return NSLOCTEXT("PlayerAttributeHUD", "ConversionEnergyNoneFormat", "Energy: None");
	}

	return FText::Format(
		NSLOCTEXT("PlayerAttributeHUD", "ConversionEnergyHeldFormat", "Energy: {0}"),
		GetConversionEnergyTypeDisplayName(ConversionEnergyComponent->GetHeldConversionEnergyType()));
}

FText UPlayerAttributeHUDWidget::GetDungeonTimerText() const
{
	const int32 ElapsedSeconds = DungeonRunManager ? DungeonRunManager->GetCurrentRunElapsedSeconds() : 0;
	const int32 Minutes = ElapsedSeconds / 60;
	const int32 Seconds = ElapsedSeconds % 60;
	return FText::FromString(FString::Printf(TEXT("Run Time: %02d:%02d"), Minutes, Seconds));
}

void UPlayerAttributeHUDWidget::OpenSettingsMenu()
{
	if (!SettingsMenuClass)
	{
		SettingsMenuClass = USettingsMenuWidget::StaticClass();
	}

	if (!SettingsMenu && SettingsMenuClass)
	{
		SettingsMenu = CreateWidget<USettingsMenuWidget>(GetOwningPlayer(), SettingsMenuClass);
		if (SettingsMenu)
		{
			SettingsMenu->OnSettingsBackRequested.RemoveDynamic(this, &UPlayerAttributeHUDWidget::HandleSettingsBackRequested);
			SettingsMenu->OnSettingsBackRequested.AddDynamic(this, &UPlayerAttributeHUDWidget::HandleSettingsBackRequested);
		}
	}

	if (SettingsMenu)
	{
		SettingsMenu->RefreshFromAudioSettings();
		if (!SettingsMenu->IsInViewport())
		{
			SettingsMenu->AddToViewport(50);
		}
	}
}

void UPlayerAttributeHUDWidget::CloseSettingsMenu()
{
	if (!SettingsMenu)
	{
		return;
	}

	SettingsMenu->OnSettingsBackRequested.RemoveDynamic(this, &UPlayerAttributeHUDWidget::HandleSettingsBackRequested);
	SettingsMenu->RemoveFromParent();
	SettingsMenu = nullptr;
}

void UPlayerAttributeHUDWidget::HandlePlayerAttributeChanged(int32 NewConstructValue, int32 NewAcidValue)
{
	// Values are pulled from the component to keep one formatting path for initial and event refreshes.
	RefreshAttributeDisplay();
}

void UPlayerAttributeHUDWidget::HandlePlayerHealthChanged(int32 NewHealth, int32 MaxHealth)
{
	RefreshAttributeDisplay();
}

void UPlayerAttributeHUDWidget::HandleConversionEnergyChanged(bool bHasEnergy, ETileType EnergyType)
{
	RefreshConversionEnergyDisplay();
}

void UPlayerAttributeHUDWidget::HandleDungeonLevelStarted(int32 LevelIndex)
{
	if (DungeonRunManager)
	{
		BindToEnemyManager(DungeonRunManager->EnemyManager);
	}

	RefreshDungeonDisplay();
	RefreshEnemyCountDisplay();
	StartDungeonRunTimerRefresh();
}

void UPlayerAttributeHUDWidget::HandleDungeonLevelCompleted(int32 CompletedLevelIndex, int32 NextLevelIndex)
{
	RefreshDungeonDisplay();
	RefreshEnemyCountDisplay();
}

void UPlayerAttributeHUDWidget::HandleDungeonRunEnded(int32 FinalLevelIndex, int32 ElapsedSeconds)
{
	RefreshDungeonDisplay();
	StopDungeonRunTimerRefresh();
}

void UPlayerAttributeHUDWidget::HandleEnemyCountChanged(int32 AliveEnemyCount)
{
	RefreshEnemyCountDisplay();
}

void UPlayerAttributeHUDWidget::HandleSettingButtonClicked()
{
	OpenSettingsMenu();
}

void UPlayerAttributeHUDWidget::HandleSettingsBackRequested()
{
	CloseSettingsMenu();
}

void UPlayerAttributeHUDWidget::BuildFallbackWidgetTreeIfNeeded()
{
	if (!WidgetTree)
	{
		return;
	}

	if (HealthText || HealthProgressBar || ConstructText || AcidText || ConstructProgressBar || AcidProgressBar || EnergyText || LevelText || TimerText || EnemyCountText)
	{
		if (!LevelText)
		{
			LevelText = CreateFallbackTextBlock(TEXT("LevelText"), NSLOCTEXT("PlayerAttributeHUD", "LevelInitialText", "Level: 1"));
		}
		if (!TimerText)
		{
			TimerText = CreateFallbackTextBlock(TEXT("TimerText"), NSLOCTEXT("PlayerAttributeHUD", "TimerInitialText", "Run Time: 00:00"));
		}
		if (!EnemyCountText)
		{
			EnemyCountText = CreateFallbackTextBlock(TEXT("EnemyCountText"), NSLOCTEXT("PlayerAttributeHUD", "EnemyCountInitialText", "Enemies: 0"));
		}
		if (!SettingButton && WidgetTree && WidgetTree->RootWidget)
		{
			SettingButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SettingButton"));
			UTextBlock* SettingLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingButtonLabel"));
			if (SettingButton)
			{
				if (SettingLabel)
				{
					SettingLabel->SetText(NSLOCTEXT("PlayerAttributeHUD", "SettingButtonLabel", "Setting"));
					SettingButton->AddChild(SettingLabel);
				}

				if (UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget))
				{
					RootPanel->AddChild(SettingButton);
				}
			}
		}
		return;
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AttributeRoot"));
	WidgetTree->RootWidget = RootBox;

	LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LevelText"));
	TimerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TimerText"));
	EnemyCountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EnemyCountText"));
	HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
	HealthProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthProgressBar"));
	ConstructText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConstructText"));
	ConstructProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ConstructProgressBar"));
	AcidText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AcidText"));
	AcidProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("AcidProgressBar"));
	EnergyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EnergyText"));
	SettingButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SettingButton"));
	UTextBlock* SettingLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingButtonLabel"));

	if (LevelText)
	{
		LevelText->SetText(NSLOCTEXT("PlayerAttributeHUD", "LevelInitialText", "Level: 1"));
		RootBox->AddChildToVerticalBox(LevelText);
	}

	if (TimerText)
	{
		TimerText->SetText(NSLOCTEXT("PlayerAttributeHUD", "TimerInitialText", "Run Time: 00:00"));
		RootBox->AddChildToVerticalBox(TimerText);
	}

	if (EnemyCountText)
	{
		EnemyCountText->SetText(NSLOCTEXT("PlayerAttributeHUD", "EnemyCountInitialText", "Enemies: 0"));
		RootBox->AddChildToVerticalBox(EnemyCountText);
	}

	if (HealthText)
	{
		HealthText->SetText(NSLOCTEXT("PlayerAttributeHUD", "HealthInitialText", "HP: 3 / 3"));
		RootBox->AddChildToVerticalBox(HealthText);
	}

	if (HealthProgressBar)
	{
		RootBox->AddChildToVerticalBox(HealthProgressBar);
	}

	if (ConstructText)
	{
		ConstructText->SetText(NSLOCTEXT("PlayerAttributeHUD", "ConstructInitialText", "Construct: 0 / 10"));
		RootBox->AddChildToVerticalBox(ConstructText);
	}

	if (ConstructProgressBar)
	{
		RootBox->AddChildToVerticalBox(ConstructProgressBar);
	}

	if (AcidText)
	{
		AcidText->SetText(NSLOCTEXT("PlayerAttributeHUD", "AcidInitialText", "Acid: 0 / 10"));
		RootBox->AddChildToVerticalBox(AcidText);
	}

	if (AcidProgressBar)
	{
		RootBox->AddChildToVerticalBox(AcidProgressBar);
	}

	if (EnergyText)
	{
		EnergyText->SetText(NSLOCTEXT("PlayerAttributeHUD", "EnergyInitialText", "Energy: None"));
		RootBox->AddChildToVerticalBox(EnergyText);
	}

	if (SettingButton)
	{
		if (SettingLabel)
		{
			SettingLabel->SetText(NSLOCTEXT("PlayerAttributeHUD", "SettingButtonLabel", "Setting"));
			SettingButton->AddChild(SettingLabel);
		}
		RootBox->AddChildToVerticalBox(SettingButton);
	}
}

UTextBlock* UPlayerAttributeHUDWidget::CreateFallbackTextBlock(const FName& WidgetName, const FText& InitialText)
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), WidgetName);
	if (!TextBlock)
	{
		return nullptr;
	}

	TextBlock->SetText(InitialText);

	if (UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget))
	{
		RootPanel->AddChild(TextBlock);
	}

	return TextBlock;
}

void UPlayerAttributeHUDWidget::BindToAttributeComponent(UPlayerAttributeComponent* InAttributeComponent)
{
	if (AttributeComponent == InAttributeComponent)
	{
		return;
	}

	UnbindFromAttributeComponent();
	AttributeComponent = InAttributeComponent;

	if (AttributeComponent)
	{
		// Event-driven HUD refresh: no Tick polling and no direct gameplay writes from UI.
		AttributeComponent->OnPlayerAttributeChanged.AddDynamic(this, &UPlayerAttributeHUDWidget::HandlePlayerAttributeChanged);
		AttributeComponent->OnPlayerHealthChanged.AddDynamic(this, &UPlayerAttributeHUDWidget::HandlePlayerHealthChanged);
	}
}

void UPlayerAttributeHUDWidget::UnbindFromAttributeComponent()
{
	if (AttributeComponent)
	{
		// Remove dynamic binding during widget teardown to avoid callbacks to a dead widget.
		AttributeComponent->OnPlayerAttributeChanged.RemoveDynamic(this, &UPlayerAttributeHUDWidget::HandlePlayerAttributeChanged);
		AttributeComponent->OnPlayerHealthChanged.RemoveDynamic(this, &UPlayerAttributeHUDWidget::HandlePlayerHealthChanged);
		AttributeComponent = nullptr;
	}
}

void UPlayerAttributeHUDWidget::BindToConversionEnergyComponent(UConversionEnergyComponent* InEnergyComponent)
{
	if (ConversionEnergyComponent == InEnergyComponent)
	{
		return;
	}

	UnbindFromConversionEnergyComponent();
	ConversionEnergyComponent = InEnergyComponent;

	if (ConversionEnergyComponent)
	{
		ConversionEnergyComponent->OnConversionEnergyChanged.AddDynamic(this, &UPlayerAttributeHUDWidget::HandleConversionEnergyChanged);
	}
}

void UPlayerAttributeHUDWidget::UnbindFromConversionEnergyComponent()
{
	if (ConversionEnergyComponent)
	{
		ConversionEnergyComponent->OnConversionEnergyChanged.RemoveDynamic(this, &UPlayerAttributeHUDWidget::HandleConversionEnergyChanged);
		ConversionEnergyComponent = nullptr;
	}
}

void UPlayerAttributeHUDWidget::BindToDungeonRunManager(ADungeonRunManager* InDungeonRunManager)
{
	if (DungeonRunManager == InDungeonRunManager)
	{
		return;
	}

	UnbindFromDungeonRunManager();
	DungeonRunManager = InDungeonRunManager;

	if (DungeonRunManager)
	{
		DungeonRunManager->OnDungeonLevelStarted.AddDynamic(this, &UPlayerAttributeHUDWidget::HandleDungeonLevelStarted);
		DungeonRunManager->OnDungeonLevelCompleted.AddDynamic(this, &UPlayerAttributeHUDWidget::HandleDungeonLevelCompleted);
		DungeonRunManager->OnDungeonRunEnded.AddDynamic(this, &UPlayerAttributeHUDWidget::HandleDungeonRunEnded);
		BindToEnemyManager(DungeonRunManager->EnemyManager);
	}
}

void UPlayerAttributeHUDWidget::UnbindFromDungeonRunManager()
{
	if (DungeonRunManager)
	{
		UnbindFromEnemyManager();
		DungeonRunManager->OnDungeonLevelStarted.RemoveDynamic(this, &UPlayerAttributeHUDWidget::HandleDungeonLevelStarted);
		DungeonRunManager->OnDungeonLevelCompleted.RemoveDynamic(this, &UPlayerAttributeHUDWidget::HandleDungeonLevelCompleted);
		DungeonRunManager->OnDungeonRunEnded.RemoveDynamic(this, &UPlayerAttributeHUDWidget::HandleDungeonRunEnded);
		DungeonRunManager = nullptr;
	}
}

void UPlayerAttributeHUDWidget::BindToEnemyManager(AGridEnemyManager* InEnemyManager)
{
	if (EnemyManager == InEnemyManager)
	{
		RefreshEnemyCountDisplay();
		return;
	}

	UnbindFromEnemyManager();
	EnemyManager = InEnemyManager;

	if (EnemyManager)
	{
		EnemyManager->OnEnemyCountChanged.AddDynamic(this, &UPlayerAttributeHUDWidget::HandleEnemyCountChanged);
	}

	RefreshEnemyCountDisplay();
}

void UPlayerAttributeHUDWidget::UnbindFromEnemyManager()
{
	if (EnemyManager)
	{
		EnemyManager->OnEnemyCountChanged.RemoveDynamic(this, &UPlayerAttributeHUDWidget::HandleEnemyCountChanged);
		EnemyManager = nullptr;
	}
}

void UPlayerAttributeHUDWidget::StartDungeonRunTimerRefresh()
{
	UWorld* World = GetWorld();
	if (!World || !DungeonRunManager || !DungeonRunManager->IsRunTimerRunning())
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		DungeonTimerRefreshHandle,
		this,
		&UPlayerAttributeHUDWidget::RefreshDungeonDisplay,
		1.0f,
		true);
}

void UPlayerAttributeHUDWidget::StopDungeonRunTimerRefresh()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DungeonTimerRefreshHandle);
	}
}

UMaterialInstanceDynamic* UPlayerAttributeHUDWidget::RefreshAttributeImage(
	UImage* AttributeImage,
	UMaterialInstanceDynamic* AttributeMaterial,
	int32 AttributeValue,
	int32 MaxAttributeValue,
	float EnemyTypeValue)
{
	if (!AttributeImage)
	{
		return nullptr;
	}

	if (!AttributeMaterial)
	{
		AttributeMaterial = AttributeImage->GetDynamicMaterial();
	}

	if (!AttributeMaterial)
	{
		return nullptr;
	}

	const int32 SafeMaxAttributeValue = FMath::Max(MaxAttributeValue, 1);
	const int32 ClampedAttributeValue = FMath::Clamp(AttributeValue, 0, SafeMaxAttributeValue);
	const float bFullValue = ClampedAttributeValue >= SafeMaxAttributeValue ? 1.f : 0.f;
	const float OffsetValue = static_cast<float>(FMath::Clamp(ClampedAttributeValue, 0, SafeMaxAttributeValue - 1));

	AttributeMaterial->SetScalarParameterValue(EnemyTypeParameterName, EnemyTypeValue);
	AttributeMaterial->SetScalarParameterValue(FullParameterName, bFullValue);
	AttributeMaterial->SetScalarParameterValue(OffsetXParameterName, OffsetValue);

	return AttributeMaterial;
}
