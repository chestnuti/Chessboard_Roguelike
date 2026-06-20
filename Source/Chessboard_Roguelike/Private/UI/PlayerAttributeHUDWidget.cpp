#include "UI/PlayerAttributeHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/ConversionEnergyComponent.h"
#include "Player/PlayerAttributeComponent.h"

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

	// Gives the native class a minimal visible HUD even if the Widget Blueprint has no designer tree yet.
	BuildFallbackWidgetTreeIfNeeded();

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
}

void UPlayerAttributeHUDWidget::NativeDestruct()
{
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

void UPlayerAttributeHUDWidget::BuildFallbackWidgetTreeIfNeeded()
{
	if (!WidgetTree || HealthText || HealthProgressBar || ConstructText || AcidText || ConstructProgressBar || AcidProgressBar || EnergyText)
	{
		// Widget Blueprints with named bindings keep their authored layout.
		return;
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AttributeRoot"));
	WidgetTree->RootWidget = RootBox;

	HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
	HealthProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthProgressBar"));
	ConstructText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConstructText"));
	ConstructProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ConstructProgressBar"));
	AcidText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AcidText"));
	AcidProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("AcidProgressBar"));
	EnergyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EnergyText"));

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
