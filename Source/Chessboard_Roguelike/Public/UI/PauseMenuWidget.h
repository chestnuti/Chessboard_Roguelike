#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPauseMenuActionRequested);

UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Pause|Events")
	FOnPauseMenuActionRequested OnResumeRequested;

	UPROPERTY(BlueprintAssignable, Category = "Pause|Events")
	FOnPauseMenuActionRequested OnBackRequested;

	UPROPERTY(BlueprintAssignable, Category = "Pause|Events")
	FOnPauseMenuActionRequested OnSettingsRequested;

	UPROPERTY(BlueprintAssignable, Category = "Pause|Events")
	FOnPauseMenuActionRequested OnMainMenuRequested;

	UPROPERTY(BlueprintAssignable, Category = "Pause|Events")
	FOnPauseMenuActionRequested OnQuitGameRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause")
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause")
	TObjectPtr<UButton> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause")
	TObjectPtr<UButton> Back;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause")
	TObjectPtr<UButton> SettingButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause")
	TObjectPtr<UButton> MainMenuButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause")
	TObjectPtr<UButton> QuitGameButton;

private:
	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleSettingClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

	UFUNCTION()
	void HandleQuitGameClicked();

	void BuildFallbackWidgetTreeIfNeeded();
};
