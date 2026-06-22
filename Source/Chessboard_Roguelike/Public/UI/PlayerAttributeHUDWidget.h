#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Grid/GridTypes.h"
#include "TimerManager.h"
#include "PlayerAttributeHUDWidget.generated.h"

class ADungeonRunManager;
class AGridEnemyManager;
class UButton;
class UConversionEnergyComponent;
class UImage;
class UMaterialInstanceDynamic;
class UPlayerAttributeComponent;
class UProgressBar;
class USettingsMenuWidget;
class UTextBlock;

// Read-only attribute HUD that refreshes from UPlayerAttributeComponent events instead of Tick.
UCLASS(Blueprintable)
class CHESSBOARD_ROGUELIKE_API UPlayerAttributeHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Allows the controller to bind the HUD before NativeConstruct discovers the owning pawn.
	UFUNCTION(BlueprintCallable, Category = "Player Attributes")
	void InitializeFromAttributeComponent(UPlayerAttributeComponent* InAttributeComponent);

	// Allows the controller or Blueprint to bind the HUD to the player's conversion energy component.
	UFUNCTION(BlueprintCallable, Category = "Conversion Energy")
	void InitializeFromConversionEnergyComponent(UConversionEnergyComponent* InEnergyComponent);

	// Allows the controller or Blueprint to bind the HUD to the active dungeon run.
	UFUNCTION(BlueprintCallable, Category = "Dungeon")
	void InitializeFromDungeonRunManager(ADungeonRunManager* InDungeonRunManager);

	// Safe to call from events or Blueprint when the displayed values need a refresh.
	UFUNCTION(BlueprintCallable, Category = "Player Attributes")
	void RefreshAttributeDisplay();

	UFUNCTION(BlueprintCallable, Category = "Conversion Energy")
	void RefreshConversionEnergyDisplay();

	UFUNCTION(BlueprintCallable, Category = "Dungeon")
	void RefreshDungeonDisplay();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void RefreshEnemyCountDisplay();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Conversion Energy")
	FText GetConversionEnergyStatusText() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dungeon")
	FText GetDungeonTimerText() const;

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void OpenSettingsMenu();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void CloseSettingsMenu();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Optional bindings match the Widget Blueprint names; native fallback widgets are created if they are absent.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UTextBlock> ConstructText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UTextBlock> AcidText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UProgressBar> ConstructProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UProgressBar> AcidProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UImage> ConstructAttribute;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Player Attributes")
	TObjectPtr<UImage> AcidAttribute;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Conversion Energy")
	TObjectPtr<UTextBlock> EnergyText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dungeon")
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Dungeon")
	TObjectPtr<UTextBlock> TimerText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Enemy")
	TObjectPtr<UTextBlock> EnemyCountText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings")
	TSubclassOf<USettingsMenuWidget> SettingsMenuClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings")
	TObjectPtr<UButton> SettingButton;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPlayerAttributeComponent> AttributeComponent;

	UPROPERTY(Transient)
	TObjectPtr<UConversionEnergyComponent> ConversionEnergyComponent;

	UPROPERTY(Transient)
	TObjectPtr<ADungeonRunManager> DungeonRunManager;

	UPROPERTY(Transient)
	TObjectPtr<AGridEnemyManager> EnemyManager;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ConstructAttributeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> AcidAttributeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<USettingsMenuWidget> SettingsMenu;

	FTimerHandle DungeonTimerRefreshHandle;

	UFUNCTION()
	void HandlePlayerAttributeChanged(int32 NewConstructValue, int32 NewAcidValue);

	UFUNCTION()
	void HandlePlayerHealthChanged(int32 NewHealth, int32 MaxHealth);

	UFUNCTION()
	void HandleConversionEnergyChanged(bool bHasEnergy, ETileType EnergyType);

	UFUNCTION()
	void HandleDungeonLevelStarted(int32 LevelIndex);

	UFUNCTION()
	void HandleDungeonLevelCompleted(int32 CompletedLevelIndex, int32 NextLevelIndex);

	UFUNCTION()
	void HandleDungeonRunEnded(int32 FinalLevelIndex, int32 ElapsedSeconds);

	UFUNCTION()
	void HandleEnemyCountChanged(int32 AliveEnemyCount);

	UFUNCTION()
	void HandleSettingButtonClicked();

	UFUNCTION()
	void HandleSettingsBackRequested();

	void BuildFallbackWidgetTreeIfNeeded();
	void BindToAttributeComponent(UPlayerAttributeComponent* InAttributeComponent);
	void UnbindFromAttributeComponent();
	void BindToConversionEnergyComponent(UConversionEnergyComponent* InEnergyComponent);
	void UnbindFromConversionEnergyComponent();
	void BindToDungeonRunManager(ADungeonRunManager* InDungeonRunManager);
	void UnbindFromDungeonRunManager();
	void BindToEnemyManager(AGridEnemyManager* InEnemyManager);
	void UnbindFromEnemyManager();
	void StartDungeonRunTimerRefresh();
	void StopDungeonRunTimerRefresh();
	UTextBlock* CreateFallbackTextBlock(const FName& WidgetName, const FText& InitialText);
	UMaterialInstanceDynamic* RefreshAttributeImage(UImage* AttributeImage, UMaterialInstanceDynamic* AttributeMaterial, int32 AttributeValue, int32 MaxAttributeValue, float EnemyTypeValue);
};
