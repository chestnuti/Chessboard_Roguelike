#pragma once

#include "CoreMinimal.h"
#include "Audio/GameAudioTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameAudioSubsystem.generated.h"

class UAudioComponent;
class UEnemyAudioProfileDataAsset;
class UGameAudioSettingsDataAsset;
class USoundBase;

UCLASS(BlueprintType)
class CHESSBOARD_ROGUELIKE_API UGameAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Settings")
	void SetAudioSettings(UGameAudioSettingsDataAsset* InAudioSettings);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game Audio|Settings")
	UGameAudioSettingsDataAsset* GetAudioSettings() const;

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	void SetBGMVolume(float InVolume);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game Audio|Settings")
	float GetBGMVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	void SetSFXVolume(float InVolume);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game Audio|Settings")
	float GetSFXVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Game Audio|BGM")
	void PlayMainMenuBGM(float FadeTime = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Game Audio|BGM")
	void PlayLevelBGM(float FadeTime = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Game Audio|BGM")
	void PlayBGM(USoundBase* Music, float FadeTime = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Game Audio|BGM")
	void StopBGM(float FadeTime = -1.0f);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game Audio|BGM")
	USoundBase* GetCurrentBGM() const;

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Player")
	void PlayPlayerAttackSFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Player")
	void PlayPlayerKillSFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Player")
	void PlayPlayerTransformSFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Player")
	void PlayPlayerGainEnergySFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Player")
	void StartPlayerPrepareUseEnergySFX(float FadeInTime = 0.0f, bool bRestartIfPlaying = false);

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Player")
	void StopPlayerPrepareUseEnergySFX(float FadeOutTime = 0.0f);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game Audio|Player")
	bool IsPlayerPrepareUseEnergySFXPlaying() const;

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Player")
	void PlayPlayerUseEnergySFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Player")
	void PlayPlayerSwitchEnergySFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Player")
	void PlayPlayerPickupItemSFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Player")
	void PlayPlayerGainAttributeSFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Player")
	void PlayPlayerAttributeFullSFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Player")
	void PlayPlayerDeathSFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Enemy")
	void PlayEnemyMeleeAttackSFX(UEnemyAudioProfileDataAsset* AudioProfile, FVector WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Enemy")
	void PlayEnemyMeleeDeathSFX(UEnemyAudioProfileDataAsset* AudioProfile, FVector WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Enemy")
	void PlayEnemyRangedAimSFX(UEnemyAudioProfileDataAsset* AudioProfile, FVector WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Enemy")
	void PlayEnemyRangedAttackSFX(UEnemyAudioProfileDataAsset* AudioProfile, FVector WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Enemy")
	void PlayEnemyRangedDeathSFX(UEnemyAudioProfileDataAsset* AudioProfile, FVector WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Level")
	void PlayLevelStartSFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Level")
	void PlayLevelFailedSFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Level")
	void PlayLevelVictorySFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|Level")
	void PlayLevelTransitionSFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|UI")
	void PlayUIHoverSFX();

	UFUNCTION(BlueprintCallable, Category = "Game Audio|UI")
	void PlayUIClickSFX();

private:
	UFUNCTION()
	void HandleBGMFinished();

	UFUNCTION()
	void HandlePersistentSFXFinished();

	UPROPERTY(Transient)
	TObjectPtr<UGameAudioSettingsDataAsset> AudioSettings;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentBGMComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CurrentBGM;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> PlayerPrepareUseEnergyComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> ActivePersistentSFXComponents;

	UPROPERTY(Transient)
	float BGMVolume = 1.0f;

	UPROPERTY(Transient)
	float SFXVolume = 1.0f;

	float ResolveFadeTime(float FadeTime) const;
	USoundBase* SelectRandomSound(const FGameSoundSet& SoundSet) const;
	UAudioComponent* CreatePersistent2DSFXComponent(USoundBase* Sound);
	void Play2DSFX(const FGameSoundSet& SoundSet);
	void PlayWorldSFX(const FGameSoundSet& SoundSet, FVector WorldLocation);
	void TrackPersistentSFXComponent(UAudioComponent* AudioComponent);
	void PruneFinishedPersistentSFXComponents();
};
