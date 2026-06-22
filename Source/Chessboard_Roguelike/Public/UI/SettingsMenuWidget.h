#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsMenuWidget.generated.h"

class UButton;
class USlider;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsBackRequested);

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API USettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Settings|Events")
	FOnSettingsBackRequested OnSettingsBackRequested;

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void RefreshFromAudioSettings();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings")
	TObjectPtr<USlider> BGMSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings")
	TObjectPtr<USlider> SFXSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings")
	TObjectPtr<UButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings")
	TObjectPtr<UTextBlock> BGMVolumeText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings")
	TObjectPtr<UTextBlock> SFXVolumeText;

private:
	UFUNCTION()
	void HandleBGMVolumeChanged(float Value);

	UFUNCTION()
	void HandleSFXVolumeChanged(float Value);

	UFUNCTION()
	void HandleBackClicked();

	void BuildFallbackWidgetTreeIfNeeded();
	void SetSliderValueSilently(USlider* Slider, float Value);
	void RefreshVolumeLabels();
};
